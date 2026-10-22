// SPDX-License-Identifier: GPL-2.0
/*
 * Selftest for the Kestrel per-task scheduler hint.
 *
 * Build against the patched kernel's headers (or the Kestrel libkestrel-hint
 * header, which mirrors them) and run on the patched kernel. It sets a hint on
 * itself and reads it back.
 *
 *   cc -O2 -Wall -o selftest-kestrel-hint selftest-kestrel-hint.c
 *   ./selftest-kestrel-hint
 *
 * Exit 0 on success, 1 on failure, 77 (automake "skip") when the kernel has no
 * hint support, so a stock kernel does not fail the suite.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef SCHED_FLAG_KESTREL_HINT
#define SCHED_FLAG_KESTREL_HINT (1ULL << 40)
#endif
#define KESTREL_HINT_MAX 1024u

struct sched_attr_ext {
	uint32_t size;
	uint32_t sched_policy;
	uint64_t sched_flags;
	int32_t sched_nice;
	uint32_t sched_priority;
	uint64_t sched_runtime;
	uint64_t sched_deadline;
	uint64_t sched_period;
	uint32_t sched_util_min;
	uint32_t sched_util_max;
	uint32_t sched_ext_hint;
};

static int hint_set(pid_t pid, uint32_t hint)
{
	struct sched_attr_ext a;

	memset(&a, 0, sizeof(a));
	a.size = sizeof(a);
	a.sched_flags = SCHED_FLAG_KESTREL_HINT |
			/* KEEP_POLICY */ 0x08ULL | /* KEEP_PARAMS */ 0x10ULL;
	a.sched_ext_hint = hint;
	return (int)syscall(SYS_sched_setattr, pid, &a, 0u);
}

static int hint_get(pid_t pid, uint32_t *hint)
{
	struct sched_attr_ext a;

	memset(&a, 0, sizeof(a));
	a.size = sizeof(a);
	if (syscall(SYS_sched_getattr, pid, &a, sizeof(a), 0u) != 0)
		return -1;
	if (a.size < sizeof(a))
		return -1; /* kernel has no hint field */
	*hint = a.sched_ext_hint;
	return 0;
}

int main(void)
{
	uint32_t got = 0;
	int rc;

	/* Probe: a stock kernel rejects the unknown flag. */
	rc = hint_set(0, 7);
	if (rc != 0 && (errno == EINVAL || errno == ENOSYS)) {
		puts("SKIP: kernel has no Kestrel hint support");
		return 77;
	}
	if (rc != 0) {
		fprintf(stderr, "set(7) failed: %s\n", strerror(errno));
		return 1;
	}

	if (hint_get(0, &got) != 0) {
		fprintf(stderr, "get failed: %s\n", strerror(errno));
		return 1;
	}
	if (got != 7) {
		fprintf(stderr, "expected hint 7, got %u\n", got);
		return 1;
	}

	/* Upper bound is enforced. */
	if (hint_set(0, KESTREL_HINT_MAX + 1) == 0 || errno != EINVAL) {
		fprintf(stderr, "hint above max was accepted\n");
		return 1;
	}

	/* And zero clears it. */
	if (hint_set(0, 0) != 0 || hint_get(0, &got) != 0 || got != 0) {
		fprintf(stderr, "clearing the hint failed\n");
		return 1;
	}

	puts("PASS: Kestrel hint round-trips and is bounded");
	return 0;
}
