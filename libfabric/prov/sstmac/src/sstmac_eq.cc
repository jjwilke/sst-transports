/**
Copyright 2009-2020 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2020, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/
/*
 * Copyright (c) 2015-2016 Cray Inc.  All rights reserved.
 * Copyright (c) 2015 Los Alamos National Security, LLC. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <assert.h>

#include <stdlib.h>

#include "sstmac.h"


DIRECT_FN STATIC ssize_t sstmac_eq_read(struct fid_eq *eq, uint32_t *event,
              void *buf, size_t len, uint64_t flags);

DIRECT_FN STATIC ssize_t sstmac_eq_readerr(struct fid_eq *eq,
           struct fi_eq_err_entry *buf,
           uint64_t flags);

DIRECT_FN STATIC ssize_t sstmac_eq_write(struct fid_eq *eq, uint32_t event,
               const void *buf, size_t len,
               uint64_t flags);

DIRECT_FN STATIC ssize_t sstmac_eq_sread(struct fid_eq *eq, uint32_t *event,
               void *buf, size_t len, int timeout,
               uint64_t flags);

DIRECT_FN STATIC const char *sstmac_eq_strerror(struct fid_eq *eq, int prov_errno,
                const void *err_data, char *buf,
                size_t len);

DIRECT_FN STATIC extern "C" int sstmac_eq_close(struct fid *fid);

DIRECT_FN STATIC extern "C" int sstmac_eq_control(struct fid *eq, int command, void *arg);

static struct fi_ops_eq sstmac_eq_ops = {
  .size = sizeof(struct fi_ops_eq),
  .read = sstmac_eq_read,
  .readerr = sstmac_eq_readerr,
  .write = sstmac_eq_write,
  .sread = sstmac_eq_sread,
  .strerror = sstmac_eq_strerror
};

static struct fi_ops sstmac_fi_eq_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_eq_close,
  .bind = fi_no_bind,
  .control = sstmac_eq_control,
  .ops_open = fi_no_ops_open
};


DIRECT_FN extern "C" int sstmac_eq_open(struct fid_fabric *fabric, struct fi_eq_attr *attr,
			   struct fid_eq **eq, void *context)
{
	int ret = FI_SUCCESS;
 #if 0
  struct sstmac_fid_eq *eq_priv;

	SSTMAC_TRACE(FI_LOG_EQ, "\n");

	if (!fabric)
		return -FI_EINVAL;

  eq_priv = (sstmac_fid_eq*) calloc(1, sizeof(*eq_priv));
	if (!eq_priv)
		return -FI_ENOMEM;

	ret = sstmac_verify_eq_attr(attr);
	if (ret)
		goto err;

	eq_priv->fabric = container_of(fabric, struct sstmac_fid_fabric,
					  fab_fid);

	_sstmac_ref_init(&eq_priv->ref_cnt, 1, __eq_destruct);

	_sstmac_ref_get(eq_priv->fabric);

	eq_priv->eq_fid.fid.fclass = FI_CLASS_EQ;
	eq_priv->eq_fid.fid.context = context;
	eq_priv->eq_fid.fid.ops = &sstmac_fi_eq_ops;
	eq_priv->eq_fid.ops = &sstmac_eq_ops;
	eq_priv->requires_lock = 1;
	eq_priv->attr = *attr;

	fastlock_init(&eq_priv->lock);

	rwlock_init(&eq_priv->poll_obj_lock);
	dlist_init(&eq_priv->poll_objs);

	dlist_init(&eq_priv->err_bufs);

	ret = sstmac_eq_set_wait(eq_priv);
	if (ret)
		goto err1;

	ret = _sstmac_queue_create(&eq_priv->events, alloc_eq_entry,
				 free_eq_entry, 0, eq_priv->attr.size);
	if (ret)
		goto err1;

	ret = _sstmac_queue_create(&eq_priv->errors, alloc_eq_entry,
				 free_eq_entry, sizeof(struct fi_eq_err_entry),
				 0);
	if (ret)
		goto err2;

	*eq = &eq_priv->eq_fid;

	pthread_mutex_lock(&sstmac_eq_list_lock);
	dlist_insert_tail(&eq_priv->sstmac_fid_eq_list, &sstmac_eq_list);
	pthread_mutex_unlock(&sstmac_eq_list_lock);

	return ret;

err2:
	_sstmac_queue_destroy(eq_priv->events);
err1:
	_sstmac_ref_put(eq_priv->fabric);
	fastlock_destroy(&eq_priv->lock);
err:
	free(eq_priv);
#endif
	return ret;
}

DIRECT_FN STATIC extern "C" int sstmac_eq_close(struct fid *fid)
{
#if 0
	struct sstmac_fid_eq *eq;
	int references_held;

	SSTMAC_TRACE(FI_LOG_EQ, "\n");

	if (!fid)
		return -FI_EINVAL;

	eq = container_of(fid, struct sstmac_fid_eq, eq_fid);

	references_held = _sstmac_ref_put(eq);
	if (references_held) {
		SSTMAC_INFO(FI_LOG_EQ, "failed to fully close eq due "
				"to lingering references. references=%i eq=%p\n",
				references_held, eq);
	}
#endif
	return FI_SUCCESS;
}


DIRECT_FN STATIC ssize_t sstmac_eq_read(struct fid_eq *eq, uint32_t *event,
              void *buf, size_t len, uint64_t flags)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_eq_sread(struct fid_eq *eq, uint32_t *event,
               void *buf, size_t len, int timeout,
               uint64_t flags)
{
  return 0;
}

DIRECT_FN STATIC extern "C" int sstmac_eq_control(struct fid *eq, int command, void *arg)
{
  /* disabled until new trywait interface is implemented
  struct sstmac_fid_eq *eq_priv;

  eq_priv = container_of(eq, struct sstmac_fid_eq, eq_fid);
  */
  switch (command) {
  case FI_GETWAIT:
    /* return _sstmac_get_wait_obj(eq_priv->wait, arg); */
    return -FI_ENOSYS;
  default:
    return -FI_EINVAL;
  }
}

DIRECT_FN STATIC ssize_t sstmac_eq_readerr(struct fid_eq *eq,
					 struct fi_eq_err_entry *buf,
					 uint64_t flags)
{
  ssize_t read_size = sizeof(*buf);
#if 0
	struct sstmac_fid_eq *eq_priv;
	struct sstmac_eq_entry *entry;
	struct slist_entry *item;
	struct sstmac_eq_err_buf *err_buf;
	struct fi_eq_err_entry *fi_err;



	eq_priv = container_of(eq, struct sstmac_fid_eq, eq_fid);

	fastlock_acquire(&eq_priv->lock);

	if (flags & FI_PEEK)
		item = _sstmac_queue_peek(eq_priv->errors);
	else
		item = _sstmac_queue_dequeue(eq_priv->errors);

	if (!item) {
		read_size = -FI_EAGAIN;
		goto err;
	}

	entry = container_of(item, struct sstmac_eq_entry, item);
	fi_err = (struct fi_eq_err_entry *)entry->the_entry;

	memcpy(buf, entry->the_entry, read_size);

	/* If removing an event with err_data, mark err buf to be freed during
	 * the next EQ read. */
	if (!(flags & FI_PEEK) && fi_err->err_data) {
		err_buf = container_of(fi_err->err_data,
				       struct sstmac_eq_err_buf, buf);
		err_buf->do_free = 1;
	}

	_sstmac_queue_enqueue_free(eq_priv->errors, &entry->item);

err:
	fastlock_release(&eq_priv->lock);
#endif
	return read_size;
}

DIRECT_FN STATIC ssize_t sstmac_eq_write(struct fid_eq *eq, uint32_t event,
				       const void *buf, size_t len,
				       uint64_t flags)
{
  ssize_t ret = len;
#if 0
  struct sstmac_fid_eq *eq_priv;
	struct slist_entry *item;
	struct sstmac_eq_entry *entry;



	eq_priv = container_of(eq, struct sstmac_fid_eq, eq_fid);

	fastlock_acquire(&eq_priv->lock);

	item = _sstmac_queue_get_free(eq_priv->events);
	if (!item) {
		SSTMAC_WARN(FI_LOG_EQ, "error creating eq_entry\n");
		ret = -FI_ENOMEM;
		goto err;
	}

	entry = container_of(item, struct sstmac_eq_entry, item);

	entry->the_entry = calloc(1, len);
	if (!entry->the_entry) {
		_sstmac_queue_enqueue_free(eq_priv->events, &entry->item);
		SSTMAC_WARN(FI_LOG_EQ, "error allocating buffer\n");
		ret = -FI_ENOMEM;
		goto err;
	}

	memcpy(entry->the_entry, buf, len);

	entry->len = len;
	entry->type = event;
	entry->flags = flags;

	_sstmac_queue_enqueue(eq_priv->events, &entry->item);

	if (eq_priv->wait)
		_sstmac_signal_wait_obj(eq_priv->wait);

err:
	fastlock_release(&eq_priv->lock);
#endif
	return ret;
}

DIRECT_FN STATIC const char *sstmac_eq_strerror(struct fid_eq *eq, int prov_errno,
					      const void *err_data, char *buf,
					      size_t len)
{
	return NULL;
}

