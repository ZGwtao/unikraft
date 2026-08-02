/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>

#include <uk/arch/types.h>
#include <uk/boot/earlytab.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/print.h>
#include <uk/random/driver.h>

#include <uk/microkit.h>

static __u64 rs = 0x9e3779b97f4a7c15ULL;


static __u64 carrels_random_next(void)
{
	__u64 x = rs;
	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;
	rs = x;
	return x * 0x2545f4914f6cdd1dULL;
}

static int carrels_random_bytes(__u8 *buf, __sz size)
{
	__sz offset = 0;

	if (!buf && size != 0)
		return -EINVAL;

	while (offset < size) {
		__u64 value = carrels_random_next();
		__sz remaining = size - offset;
		__sz copy_size =
			remaining < sizeof(value) ? remaining : sizeof(value);

		__builtin_memcpy(buf + offset, &value, copy_size);
		offset += copy_size;
	}

	return 0;
}

static int carrels_seed_bytes(__u8 *buf, __sz size)
{
	return carrels_random_bytes(buf, size);
}

static int carrels_seed_bytes_fb(__u8 *buf, __sz size)
{
	return carrels_seed_bytes(buf, size);
}

static struct uk_random_driver_ops carrels_random_ops = {
	.random_bytes = carrels_random_bytes,
	.seed_bytes = carrels_seed_bytes,
	.seed_bytes_fb = carrels_seed_bytes_fb,
};

static struct uk_random_driver carrels_random_driver = {
	.name = "carrels-rand-d",
	.ops = &carrels_random_ops,
};

static int carrels_random_init(struct ukplat_bootinfo *bi)
{
	int rc;

	(void)bi;

	rc = uk_random_init(&carrels_random_driver);
	if (rc) {
		return rc;
	}
	return 0;
}

UK_BOOT_EARLYTAB_ENTRY(carrels_random_init, UK_RANDOM_EARLY_DRIVER_PRIO);