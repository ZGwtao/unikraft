/* SPDX-License-Identifier: BSD-2-Clause */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <uk/assert.h>
#include <uk/mutex.h>
#include <uk/print.h>
#include <microkit.h>
#include <carrels/events.h>
#include <lions/fs/config.h>
#include <lions/fs/protocol.h>
#include <vfscore/dentry.h>
#include <vfscore/file.h>
#include <vfscore/fs.h>
#include <vfscore/mount.h>
#include <vfscore/uio.h>
#include <vfscore/vnode.h>

#define SHARE_CHUNK 4096U

__attribute__((section(".fs_client_config")))
fs_client_config_t lionsfs_config;

struct lionsfs_node {
	char *path;
	uint64_t fd;
	unsigned int open_count;
};
struct lionsfs_file { uint64_t fd; };

static fs_queue_t *cmdq;
static fs_queue_t *cmplq;
static char *share;
static volatile int response_ready;
static fs_cmpl_t response;
static struct uk_mutex rpc_lock = UK_MUTEX_INITIALIZER(rpc_lock);

static int status_errno(uint64_t status)
{
	switch (status) {
	case FS_STATUS_SUCCESS: return 0;
	case FS_STATUS_NO_FILE: return ENOENT;
	case FS_STATUS_ALREADY_EXISTS: return EEXIST;
	case FS_STATUS_NOT_DIRECTORY: return ENOTDIR;
	case FS_STATUS_NOT_EMPTY: return ENOTEMPTY;
	case FS_STATUS_TOO_MANY_OPEN_FILES: return EMFILE;
	case FS_STATUS_INVALID_NAME:
	case FS_STATUS_INVALID_PATH: return EINVAL;
	case FS_STATUS_SERVER_WAS_DENIED: return EACCES;
	default: return EIO;
	}
}

static void lionsfs_event(microkit_channel ch __unused, void *arg __unused)
{
	uint64_t n = fs_queue_length_consumer(cmplq);
	for (uint64_t i = 0; i < n; i++) {
		fs_cmpl_t c = fs_queue_idx_filled(cmplq, i)->cmpl;
		if (c.id == 0) {
			response = c;
			response_ready = 1;
		}
	}
	fs_queue_publish_consumption(cmplq, n);
}

static int rpc_locked(fs_cmd_t *cmd)
{
	fs_msg_t *msg;

	cmd->id = 0;
	while (fs_queue_length_producer(cmdq) == FS_QUEUE_CAPACITY)
		uk_carrels_microkit_wait();
	msg = fs_queue_idx_empty(cmdq, 0);
	msg->cmd = *cmd;
	response_ready = 0;
	fs_queue_publish_production(cmdq, 1);
	microkit_notify(lionsfs_config.server.id);
	while (!response_ready)
		uk_carrels_microkit_wait();
	int rc = status_errno(response.status);
	if (rc && response.status != FS_STATUS_NO_FILE)
		uk_pr_err("lionsfs: command %llu failed: status=%llu errno=%d\n",
			  (unsigned long long)cmd->type,
			  (unsigned long long)response.status, rc);
	return rc;
}

static int rpc(fs_cmd_t *cmd)
{
	int rc;
	uk_mutex_lock(&rpc_lock);
	rc = rpc_locked(cmd);
	uk_mutex_unlock(&rpc_lock);
	return rc;
}

static int put_path(const char *path, uint64_t offset)
{
	size_t n = strlen(path) + 1;
	if (n > SHARE_CHUNK)
		return ENAMETOOLONG;
	memcpy(share + offset, path, n);
	return 0;
}

static char *join_path(struct vnode *dvp, const char *name)
{
	const char *base = ((struct lionsfs_node *)dvp->v_data)->path;
	size_t blen = strlen(base), nlen = strlen(name);
	char *path = malloc(blen + nlen + 2);
	if (!path) return NULL;
	if (blen == 1 && base[0] == '/')
		snprintf(path, blen + nlen + 2, "/%s", name);
	else
		snprintf(path, blen + nlen + 2, "%s/%s", base, name);
	return path;
}

static uint64_t path_ino(const char *s)
{
	uint64_t h = 1469598103934665603ULL;
	while (*s) { h ^= (unsigned char)*s++; h *= 1099511628211ULL; }
	return h ?: 1;
}

static int remote_stat(const char *path, fs_stat_t *st)
{
	fs_cmd_t cmd = { .type = FS_CMD_STAT };
	int rc;
	uk_mutex_lock(&rpc_lock);
	rc = put_path(path, 0);
	if (!rc) {
		cmd.params.stat.path = (fs_buffer_t){0, strlen(path) + 1};
		cmd.params.stat.buf = (fs_buffer_t){SHARE_CHUNK, sizeof(*st)};
		rc = rpc_locked(&cmd);
		if (!rc) memcpy(st, share + SHARE_CHUNK, sizeof(*st));
	}
	uk_mutex_unlock(&rpc_lock);
	return rc;
}

static int remote_path_cmd(uint64_t type, const char *path)
{
	fs_cmd_t cmd = { .type = type };
	int rc;
	uk_mutex_lock(&rpc_lock);
	rc = put_path(path, 0);
	if (!rc) {
		fs_buffer_t b = {0, strlen(path) + 1};
		if (type == FS_CMD_FILE_REMOVE) cmd.params.file_remove.path = b;
		else if (type == FS_CMD_DIR_REMOVE) cmd.params.dir_remove.path = b;
		else cmd.params.dir_create.path = b;
		rc = rpc_locked(&cmd);
	}
	uk_mutex_unlock(&rpc_lock);
	return rc;
}

static int lionsfs_mount(struct mount *mp, const char *dev __unused,
			 int flags __unused, const void *data __unused)
{
	struct lionsfs_node *root;
	fs_cmd_t cmd = { .type = FS_CMD_INITIALISE };
	int rc;

	if (!fs_config_check_magic(&lionsfs_config)) return ENODEV;
	cmdq = lionsfs_config.server.command_queue.vaddr;
	cmplq = lionsfs_config.server.completion_queue.vaddr;
	share = lionsfs_config.server.share.vaddr;
	rc = carrels_event_register(lionsfs_config.server.id, lionsfs_event, NULL);
	if (rc) return EBUSY;
	rc = rpc(&cmd);
	if (rc) return rc;
	root = calloc(1, sizeof(*root));
	if (!root) return ENOMEM;
	root->path = strdup("/");
	if (!root->path) { free(root); return ENOMEM; }
	mp->m_root->d_vnode->v_data = root;
	mp->m_root->d_vnode->v_type = VDIR;
	mp->m_root->d_vnode->v_mode = 0777;
	return 0;
}

static int lionsfs_unmount(struct mount *mp, int flags __unused)
{
	fs_cmd_t cmd = { .type = FS_CMD_DEINITIALISE };
	int rc = rpc(&cmd);
	carrels_event_unregister(lionsfs_config.server.id, lionsfs_event, NULL);
	vfscore_release_mp_dentries(mp);
	return rc;
}

static int lionsfs_lookup(struct vnode *dvp, const char *name, struct vnode **vpp)
{
	char *path = join_path(dvp, name);
	struct lionsfs_node *node;
	struct vnode *vp = NULL;
	fs_stat_t st;
	int rc;
	if (!path) return ENOMEM;
	rc = remote_stat(path, &st);
	if (rc) { free(path); return rc; }
	if (vfscore_vget(dvp->v_mount, path_ino(path), &vp) && vp->v_data) {
		free(path); *vpp = vp; return 0;
	}
	if (!vp) { free(path); return ENOMEM; }
	node = malloc(sizeof(*node));
	if (!node) { free(path); return ENOMEM; }
	node->path = path; vp->v_data = node;
	vp->v_type = S_ISDIR(st.mode) ? VDIR : VREG;
	/* The current FAT server reports every regular file as 0444 even when
	 * the volume and file are writable.  The RPC open/write operations remain
	 * authoritative, so expose writable POSIX mode until the protocol carries
	 * an explicit read-only attribute. */
	vp->v_mode = S_ISDIR(st.mode) ? 0777 : 0666; vp->v_size = st.size;
	*vpp = vp;
	return 0;
}

static uint64_t open_flags(int fflags, int create)
{
	int flags = vfscore_oflags(fflags), acc = flags & O_ACCMODE;
	uint64_t out = acc == O_WRONLY ? FS_OPEN_FLAGS_WRITE_ONLY :
		acc == O_RDWR ? FS_OPEN_FLAGS_READ_WRITE : FS_OPEN_FLAGS_READ_ONLY;
	return out | (create ? FS_OPEN_FLAGS_CREATE : 0);
}

static int open_path(const char *path, uint64_t flags, uint64_t *fd)
{
	fs_cmd_t cmd = { .type = FS_CMD_FILE_OPEN };
	int rc;
	uk_mutex_lock(&rpc_lock);
	rc = put_path(path, 0);
	if (!rc) {
		cmd.params.file_open.path = (fs_buffer_t){0, strlen(path) + 1};
		cmd.params.file_open.flags = flags;
		rc = rpc_locked(&cmd);
		if (!rc) *fd = response.data.file_open.fd;
	}
	uk_mutex_unlock(&rpc_lock);
	return rc;
}

static int lionsfs_open(struct vfscore_file *fp)
{
	struct lionsfs_file *file = malloc(sizeof(*file));
	struct lionsfs_node *node = fp->f_dentry->d_vnode->v_data;
	int rc;
	if (!file) return ENOMEM;
	rc = open_path(node->path, open_flags(fp->f_flags, 0), &file->fd);
	if (rc) { free(file); return rc; }
	node->fd = file->fd;
	node->open_count++;
	fp->f_data = file;
	return 0;
}

static int lionsfs_close(struct vnode *vp __unused, struct vfscore_file *fp)
{
	struct lionsfs_file *file = fp->f_data;
	struct lionsfs_node *node = fp->f_dentry->d_vnode->v_data;
	fs_cmd_t cmd = { .type = FS_CMD_FILE_CLOSE };
	int rc;
	if (!file) return 0;
	cmd.params.file_close.fd = file->fd;
	rc = rpc(&cmd); free(file); fp->f_data = NULL;
	if (node->open_count) node->open_count--;
	return rc;
}

static int lionsfs_create(struct vnode *dvp, const char *name, mode_t mode __unused)
{
	char *path = join_path(dvp, name); uint64_t fd; int rc;
	fs_cmd_t close = { .type = FS_CMD_FILE_CLOSE };
	if (!path) return ENOMEM;
	rc = open_path(path, FS_OPEN_FLAGS_READ_WRITE | FS_OPEN_FLAGS_CREATE, &fd);
	if (!rc) { close.params.file_close.fd = fd; rc = rpc(&close); }
	free(path); return rc;
}

static int lionsfs_read(struct vnode *vp, struct vfscore_file *fp,
			struct uio *uio, int flags __unused)
{
	struct lionsfs_file *file = fp->f_data;
	while (uio->uio_resid) {
		size_t want = uio->uio_resid > SHARE_CHUNK ? SHARE_CHUNK : uio->uio_resid;
		fs_cmd_t cmd = { .type = FS_CMD_FILE_READ };
		int rc;
		cmd.params.file_read.fd = file->fd; cmd.params.file_read.offset = uio->uio_offset;
		cmd.params.file_read.buf = (fs_buffer_t){0, want};
		uk_mutex_lock(&rpc_lock); rc = rpc_locked(&cmd);
		if (!rc) rc = vfscore_uiomove(share, response.data.file_read.len_read, uio);
		uk_mutex_unlock(&rpc_lock);
		if (rc || response.data.file_read.len_read < want) return rc;
	}
	return 0;
}

static int lionsfs_write(struct vnode *vp, struct uio *uio, int flags)
{
	struct lionsfs_node *node = vp->v_data; uint64_t fd; int rc;
	fs_cmd_t close = { .type = FS_CMD_FILE_CLOSE };
	int temporary = !node->open_count;
	if (flags & IO_APPEND) uio->uio_offset = vp->v_size;
	if (temporary) {
		rc = open_path(node->path, FS_OPEN_FLAGS_READ_WRITE, &fd);
		if (rc) return rc;
	} else fd = node->fd;
	while (uio->uio_resid) {
		size_t want = uio->uio_resid > SHARE_CHUNK ? SHARE_CHUNK : uio->uio_resid;
		fs_cmd_t cmd = { .type = FS_CMD_FILE_WRITE };
		uk_mutex_lock(&rpc_lock);
		rc = vfscore_uiomove(share, want, uio);
		if (!rc) {
			cmd.params.file_write.fd = fd; cmd.params.file_write.offset = uio->uio_offset - want;
			cmd.params.file_write.buf = (fs_buffer_t){0, want}; rc = rpc_locked(&cmd);
		}
		uk_mutex_unlock(&rpc_lock);
		if (rc) break;
	}
	if (temporary) {
		close.params.file_close.fd = fd;
		if (!rc) rc = rpc(&close); else (void)rpc(&close);
	}
	if (uio->uio_offset > vp->v_size) vp->v_size = uio->uio_offset;
	return rc;
}

static int lionsfs_fsync(struct vnode *vp __unused, struct vfscore_file *fp)
{
	fs_cmd_t cmd = { .type = FS_CMD_FILE_SYNC };
	cmd.params.file_sync.fd = ((struct lionsfs_file *)fp->f_data)->fd;
	return rpc(&cmd);
}

static int lionsfs_truncate(struct vnode *vp, off_t len)
{
	struct lionsfs_node *node = vp->v_data; uint64_t fd; int rc;
	int temporary = !node->open_count;
	fs_cmd_t cmd = { .type = FS_CMD_FILE_TRUNCATE }, close = { .type = FS_CMD_FILE_CLOSE };
	if (temporary) {
		rc = open_path(node->path, FS_OPEN_FLAGS_READ_WRITE, &fd);
		if (rc) return rc;
	} else fd = node->fd;
	cmd.params.file_truncate.fd = fd; cmd.params.file_truncate.length = len; rc = rpc(&cmd);
	if (temporary) { close.params.file_close.fd = fd; (void)rpc(&close); }
	if (!rc)
		vp->v_size = len;
	return rc;
}

static int lionsfs_getattr(struct vnode *vp, struct vattr *a)
{
	fs_stat_t st; int rc = remote_stat(((struct lionsfs_node *)vp->v_data)->path, &st);
	if (rc) return rc;
	a->va_nodeid = path_ino(((struct lionsfs_node *)vp->v_data)->path); a->va_size = st.size;
	a->va_type = S_ISDIR(st.mode) ? VDIR : VREG;
	a->va_mode = S_ISDIR(st.mode) ? 0777 : 0666;
	a->va_nlink = 1; a->va_nblocks = st.blocks; vp->v_size = st.size; return 0;
}

static int lionsfs_remove(struct vnode *dvp __unused, struct vnode *vp, const char *name __unused)
{ return remote_path_cmd(FS_CMD_FILE_REMOVE, ((struct lionsfs_node *)vp->v_data)->path); }
static int lionsfs_rmdir(struct vnode *dvp __unused, struct vnode *vp, const char *name __unused)
{ return remote_path_cmd(FS_CMD_DIR_REMOVE, ((struct lionsfs_node *)vp->v_data)->path); }
static int lionsfs_mkdir(struct vnode *dvp, const char *name, mode_t mode __unused)
{ char *p = join_path(dvp, name); int rc = p ? remote_path_cmd(FS_CMD_DIR_CREATE, p) : ENOMEM; free(p); return rc; }
static int lionsfs_rename(struct vnode *dvp1 __unused, struct vnode *vp1,
			  const char *name1 __unused, struct vnode *dvp2,
			  struct vnode *vp2 __unused, const char *name2)
{
	const char *oldpath = ((struct lionsfs_node *)vp1->v_data)->path;
	char *newpath = join_path(dvp2, name2);
	fs_cmd_t cmd = { .type = FS_CMD_RENAME };
	int rc;
	if (!newpath) return ENOMEM;
	uk_mutex_lock(&rpc_lock);
	rc = put_path(oldpath, 0);
	if (!rc) rc = put_path(newpath, SHARE_CHUNK);
	if (!rc) {
		cmd.params.rename.old_path = (fs_buffer_t){0, strlen(oldpath) + 1};
		cmd.params.rename.new_path = (fs_buffer_t){SHARE_CHUNK, strlen(newpath) + 1};
		rc = rpc_locked(&cmd);
	}
	uk_mutex_unlock(&rpc_lock);
	free(newpath);
	return rc;
}
static int lionsfs_inactive(struct vnode *vp)
{ struct lionsfs_node *n = vp->v_data; if (n) { free(n->path); free(n); vp->v_data = NULL; } return 0; }

#define nullop(t) ((t)vfscore_vop_nullop)
#define einval(t) ((t)vfscore_vop_einval)
#define eperm(t) ((t)vfscore_vop_eperm)
static struct vnops lionsfs_vnops = {
	.vop_open=lionsfs_open, .vop_close=lionsfs_close, .vop_read=lionsfs_read,
	.vop_write=lionsfs_write, .vop_seek=nullop(vnop_seek_t), .vop_ioctl=einval(vnop_ioctl_t),
	.vop_fsync=lionsfs_fsync, .vop_readdir=einval(vnop_readdir_t), .vop_lookup=lionsfs_lookup,
	.vop_create=lionsfs_create, .vop_remove=lionsfs_remove, .vop_rename=lionsfs_rename,
	.vop_mkdir=lionsfs_mkdir, .vop_rmdir=lionsfs_rmdir, .vop_getattr=lionsfs_getattr,
	.vop_setattr=nullop(vnop_setattr_t), .vop_inactive=lionsfs_inactive, .vop_truncate=lionsfs_truncate,
	.vop_link=eperm(vnop_link_t), .vop_cache=einval(vnop_cache_t),
	.vop_fallocate=einval(vnop_fallocate_t), .vop_readlink=einval(vnop_readlink_t),
	.vop_symlink=eperm(vnop_symlink_t), .vop_poll=einval(vnop_poll_t),
};
static struct vfsops lionsfs_vfsops = {
	.vfs_mount=lionsfs_mount, .vfs_unmount=lionsfs_unmount,
	.vfs_sync=(vfsop_sync_t)vfscore_nullop, .vfs_vget=(vfsop_vget_t)vfscore_nullop,
	.vfs_statfs=(vfsop_statfs_t)vfscore_nullop, .vfs_vnops=&lionsfs_vnops,
};
static struct vfscore_fs_type lionsfs_type = { .vs_name="lionsfs", .vs_init=NULL, .vs_op=&lionsfs_vfsops };
UK_FS_REGISTER(lionsfs_type);
