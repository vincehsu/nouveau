/*
 * Copyright 2012 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs
 */

#include <nvif/client.h>
#include <nvif/driver.h>
#include <nvif/notify.h>
#include <nvif/ioctl.h>
#include <nvif/class.h>
#include <nvif/event.h>

#include <core/ioctl.h>

#include "priv.h"

static DEFINE_MUTEX(null_mutex);
static int null_client_nr = 0;
static struct nvkm_device *null_device;
static struct pci_dev
null_pci_dev = {
	.pdev = &(struct pci_device) {
	},
};

static void
null_fini(void)
{
	nvkm_object_ref(NULL, (struct nvkm_object **)&null_device);
	nvkm_object_debug();
}

static int
null_init(const char *cfg, const char *dbg, bool init)
{
	return nvkm_device_create(&null_pci_dev, NVKM_BUS_PCI,
				  ~0ULL, "0000:00:00.0", cfg, dbg,
				  &null_device);
}

static void
null_client_unmap(void *priv, void *ptr, u32 size)
{
}

static void *
null_client_map(void *priv, u64 handle, u32 size)
{
	return NULL;
}

static int
null_client_ioctl(void *priv, bool super, void *data, u32 size, void **hack)
{
	return nvkm_ioctl(priv, super, data, size, hack);
}

static int
null_client_resume(void *priv)
{
	return nvkm_client_init(priv);
}

static int
null_client_suspend(void *priv)
{
	return nvkm_client_fini(priv, true);
}

static void
null_client_fini(void *priv)
{
	struct nvkm_object *object = priv;

	nvkm_client_fini(nv_client(object), false);
	atomic_set(&object->refcount, 1);
	nvkm_object_ref(NULL, &object);

	mutex_lock(&null_mutex);
	if (--null_client_nr == 0)
		null_fini();
	mutex_unlock(&null_mutex);
}

static int
null_client_init(const char *name, u64 device, const char *cfg,
	       const char *dbg, void **ppriv)
{
	struct nvkm_client *client;
	int ret;

	mutex_lock(&null_mutex);
	if (null_client_nr++ == 0)
		null_init(cfg, dbg, true);
	mutex_unlock(&null_mutex);

	ret = nvkm_client_create(name, ~0ULL, cfg, dbg, &client);
	*ppriv = client;
	if (ret == 0)
		client->ntfy = nvif_notify;
	return ret;
}

const struct nvif_driver
nvif_driver_null = {
	.name = "null",
	.init = null_client_init,
	.fini = null_client_fini,
	.suspend = null_client_suspend,
	.resume = null_client_resume,
	.ioctl = null_client_ioctl,
	.map = null_client_map,
	.unmap = null_client_unmap,
	.keep = false,
};
