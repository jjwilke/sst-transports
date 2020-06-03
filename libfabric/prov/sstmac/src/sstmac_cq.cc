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
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sstmac.h"

static int sstmac_cq_close(fid_t fid);
static int sstmac_cq_control(struct fid *cq, int command, void *arg);
DIRECT_FN STATIC ssize_t sstmac_cq_readfrom(struct fid_cq *cq, void *buf,
					  size_t count, fi_addr_t *src_addr);
DIRECT_FN STATIC ssize_t sstmac_cq_readerr(struct fid_cq *cq,
					 struct fi_cq_err_entry *buf,
           uint64_t flags);
DIRECT_FN STATIC ssize_t sstmac_cq_sreadfrom(struct fid_cq *cq, void *buf,
					   size_t count, fi_addr_t *src_addr,
					   const void *cond, int timeout);
DIRECT_FN extern "C" int sstmac_cq_open(struct fid_domain *domain, struct fi_cq_attr *attr,
			   struct fid_cq **cq, void *context);
DIRECT_FN STATIC const char *sstmac_cq_strerror(struct fid_cq *cq, int prov_errno,
					      const void *prov_data, char *buf,
					      size_t len);
DIRECT_FN STATIC extern "C" int sstmac_cq_signal(struct fid_cq *cq);
DIRECT_FN STATIC ssize_t sstmac_cq_read(struct fid_cq *cq,
				      void *buf,
				      size_t count);
DIRECT_FN STATIC ssize_t sstmac_cq_sread(struct fid_cq *cq, void *buf,
				       size_t count, const void *cond,
				       int timeout);

static const struct fi_ops sstmac_cq_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_cq_close,
  .bind = fi_no_bind,
  .control = sstmac_cq_control,
  .ops_open = fi_no_ops_open
};

static const struct fi_ops_cq sstmac_cq_ops = {
  .size = sizeof(struct fi_ops_cq),
  .read = sstmac_cq_read,
  .readfrom = sstmac_cq_readfrom,
  .readerr = sstmac_cq_readerr,
  .sread = sstmac_cq_sread,
  .sreadfrom = sstmac_cq_sreadfrom,
  .signal = sstmac_cq_signal,
  .strerror = sstmac_cq_strerror
};

static const size_t format_sizes[] = {
  [FI_CQ_FORMAT_UNSPEC]  = sizeof(struct fi_cq_entry),
	[FI_CQ_FORMAT_CONTEXT] = sizeof(struct fi_cq_entry),
	[FI_CQ_FORMAT_MSG]     = sizeof(struct fi_cq_msg_entry),
	[FI_CQ_FORMAT_DATA]    = sizeof(struct fi_cq_data_entry),
	[FI_CQ_FORMAT_TAGGED]  = sizeof(struct fi_cq_tagged_entry)
};

static int sstmac_cq_close(fid_t fid)
{
#if 0
  struct sstmac_fid_cq *cq;
	int references_held;

  SSTMAC_TRACE(FI_LOG_CQ, "\n");

  cq = container_of(fid, struct sstmac_fid_cq, cq_fid);

  references_held = _sstmac_ref_put(cq);

	if (references_held) {
    SSTMAC_INFO(FI_LOG_CQ, "failed to fully close cq due to lingering "
				"references. references=%i cq=%p\n",
				references_held, cq);
	}
#endif
	return FI_SUCCESS;
}

static ssize_t __sstmac_cq_readfrom(struct fid_cq *cq, void *buf,
					  size_t count, fi_addr_t *src_addr)
{
  ssize_t read_count = 0;
#if 0
  struct sstmac_fid_cq *cq_priv;
  struct sstmac_cq_entry *event;
	struct slist_entry *temp;

	if (!cq || !buf || !count)
		return -FI_EINVAL;

  cq_priv = container_of(cq, struct sstmac_fid_cq, cq_fid);

  __sstmac_cq_progress(cq_priv);

  if (_sstmac_queue_peek(cq_priv->errors))
		return -FI_EAVAIL;

	COND_ACQUIRE(cq_priv->requires_lock, &cq_priv->lock);

  while (_sstmac_queue_peek(cq_priv->events) && count--) {
    temp = _sstmac_queue_dequeue(cq_priv->events);
    event = container_of(temp, struct sstmac_cq_entry, item);

		assert(event->the_entry);
		memcpy(buf, event->the_entry, cq_priv->entry_size);
		if (src_addr)
			memcpy(&src_addr[read_count], &event->src_addr, sizeof(fi_addr_t));

    _sstmac_queue_enqueue_free(cq_priv->events, &event->item);

		buf = (void *) ((uint8_t *) buf + cq_priv->entry_size);

		read_count++;
	}

	COND_RELEASE(cq_priv->requires_lock, &cq_priv->lock);
#endif
	return read_count ?: -FI_EAGAIN;
}

static ssize_t __sstmac_cq_sreadfrom(int blocking, struct fid_cq *cq, void *buf,
				     size_t count, fi_addr_t *src_addr,
				     const void *cond, int timeout)
{
#if 0
  struct sstmac_fid_cq *cq_priv;

  cq_priv = container_of(cq, struct sstmac_fid_cq, cq_fid);
	if ((blocking && !cq_priv->wait) ||
	    (blocking && cq_priv->attr.wait_obj == FI_WAIT_SET))
		return -FI_EINVAL;

  if (_sstmac_queue_peek(cq_priv->errors))
		return -FI_EAVAIL;

	if (cq_priv->wait)
    sstmac_wait_wait((struct fid_wait *)cq_priv->wait, timeout);
#endif
  return __sstmac_cq_readfrom(cq, buf, count, src_addr);

}

DIRECT_FN STATIC ssize_t sstmac_cq_sreadfrom(struct fid_cq *cq, void *buf,
					   size_t count, fi_addr_t *src_addr,
					   const void *cond, int timeout)
{
  return __sstmac_cq_sreadfrom(1, cq, buf, count, src_addr, cond, timeout);
}

DIRECT_FN STATIC ssize_t sstmac_cq_read(struct fid_cq *cq,
				      void *buf,
				      size_t count)
{
  return __sstmac_cq_sreadfrom(0, cq, buf, count, NULL, NULL, 0);
}

DIRECT_FN STATIC ssize_t sstmac_cq_sread(struct fid_cq *cq, void *buf,
				       size_t count, const void *cond,
				       int timeout)
{
  return __sstmac_cq_sreadfrom(1, cq, buf, count, NULL, cond, timeout);
}

DIRECT_FN STATIC ssize_t sstmac_cq_readfrom(struct fid_cq *cq, void *buf,
					  size_t count, fi_addr_t *src_addr)
{
  return __sstmac_cq_sreadfrom(0, cq, buf, count, src_addr, NULL, 0);
}

DIRECT_FN STATIC ssize_t sstmac_cq_readerr(struct fid_cq *cq,
					 struct fi_cq_err_entry *buf,
					 uint64_t flags)
{
	ssize_t read_count = 0;
#if 0
  struct sstmac_fid_cq *cq_priv;
  struct sstmac_cq_entry *event;
	struct slist_entry *entry;
	size_t err_data_cpylen;
  struct fi_cq_err_entry *sstmac_cq_err;


	if (!cq || !buf)
		return -FI_EINVAL;

  cq_priv = container_of(cq, struct sstmac_fid_cq, cq_fid);

	/*
	 * we need to progress cq.  some apps may be only using
	 * cq to check for errors.
	 */

  _sstmac_prog_progress(&cq_priv->pset);

	COND_ACQUIRE(cq_priv->requires_lock, &cq_priv->lock);

  entry = _sstmac_queue_dequeue(cq_priv->errors);
	if (!entry) {
		read_count = -FI_EAGAIN;
		goto err;
	}

  event = container_of(entry, struct sstmac_cq_entry, item);
  sstmac_cq_err = (fi_cq_err_entry*) event->the_entry;

  buf->op_context = sstmac_cq_err->op_context;
  buf->flags = sstmac_cq_err->flags;
  buf->len = sstmac_cq_err->len;
  buf->buf = sstmac_cq_err->buf;
  buf->data = sstmac_cq_err->data;
  buf->tag = sstmac_cq_err->tag;
  buf->olen = sstmac_cq_err->olen;
  buf->err = sstmac_cq_err->err;
  buf->prov_errno = sstmac_cq_err->prov_errno;

  if (sstmac_cq_err->err_data != NULL) {
		/*
		 * Note: If the api version is >= 1.5 then copy err_data into
		 * buf->err_data and copy at most buf->err_data_size.
		 * If buf->err_data_size is zero or the api version is < 1.5,
		 * use the old method of allocating space in provider.
		 */
		if (FI_VERSION_LT(cq_priv->domain->fabric->fab_fid.api_version,
		    FI_VERSION(1, 5)) || buf->err_data_size == 0) {
			err_data_cpylen = sizeof(cq_priv->err_data);

      memcpy(cq_priv->err_data, sstmac_cq_err->err_data,
				err_data_cpylen);

			buf->err_data = cq_priv->err_data;
		} else {
			if (buf->err_data == NULL)
				return -FI_EINVAL;

			err_data_cpylen = MIN(buf->err_data_size,
            sstmac_cq_err->err_data_size);
      memcpy(buf->err_data, sstmac_cq_err->err_data, err_data_cpylen);
			buf->err_data_size = err_data_cpylen;
		}
    free(sstmac_cq_err->err_data);
    sstmac_cq_err->err_data = NULL;
	} else {
		if (FI_VERSION_LT(cq_priv->domain->fabric->fab_fid.api_version,
		    FI_VERSION(1, 5))) {
			buf->err_data = NULL;
		} else {
			buf->err_data_size = 0;
		}
	}

  _sstmac_queue_enqueue_free(cq_priv->errors, &event->item);

	read_count++;

err:
	COND_RELEASE(cq_priv->requires_lock, &cq_priv->lock);
#endif
	return read_count;
}

DIRECT_FN STATIC const char *sstmac_cq_strerror(struct fid_cq *cq, int prov_errno,
					      const void *prov_data, char *buf,
					      size_t len)
{
	return NULL;
}

DIRECT_FN STATIC extern "C" int sstmac_cq_signal(struct fid_cq *cq)
{
#if 0
  struct sstmac_fid_cq *cq_priv;

  cq_priv = container_of(cq, struct sstmac_fid_cq, cq_fid);

	if (cq_priv->wait)
    _sstmac_signal_wait_obj(cq_priv->wait);
#endif
	return FI_SUCCESS;
}

static int sstmac_cq_control(struct fid *cq, int command, void *arg)
{

	switch (command) {
	case FI_GETWAIT:
		return -FI_ENOSYS;
	default:
		return -FI_EINVAL;
	}
}



DIRECT_FN extern "C" int sstmac_cq_open(struct fid_domain *domain, struct fi_cq_attr *attr,
			   struct fid_cq **cq, void *context)
{
	int ret = FI_SUCCESS;
#if 0
  struct sstmac_fid_domain *domain_priv;
  struct sstmac_fid_cq *cq_priv;
	struct fi_ops_cq *cq_ops;
	struct fi_ops *fi_cq_ops;


  SSTMAC_TRACE(FI_LOG_CQ, "\n");

  cq_ops = (fi_ops_cq*) calloc(1, sizeof(*cq_ops));
	if (!cq_ops) {
		return -FI_ENOMEM;
	}

  fi_cq_ops = (fi_ops*) calloc(1, sizeof(*fi_cq_ops));
	if (!fi_cq_ops) {
		ret = -FI_ENOMEM;
		goto free_cq_ops;
	}

  *cq_ops = sstmac_cq_ops;
  *fi_cq_ops = sstmac_cq_fi_ops;

	ret = verify_cq_attr(attr, cq_ops, fi_cq_ops);
	if (ret)
		goto free_fi_cq_ops;

  domain_priv = container_of(domain, struct sstmac_fid_domain, domain_fid);
	if (!domain_priv) {
		ret = -FI_EINVAL;
		goto free_fi_cq_ops;
	}

  cq_priv = (sstmac_fid_cq*) calloc(1, sizeof(*cq_priv));
	if (!cq_priv) {
		ret = -FI_ENOMEM;
		goto free_fi_cq_ops;
	}

	cq_priv->requires_lock = (domain_priv->thread_model !=
			FI_THREAD_COMPLETION);

	cq_priv->domain = domain_priv;
	cq_priv->attr = *attr;

  _sstmac_ref_init(&cq_priv->ref_cnt, 1, __cq_destruct);
  _sstmac_ref_get(cq_priv->domain);

  _sstmac_prog_init(&cq_priv->pset);

	cq_priv->cq_fid.fid.fclass = FI_CLASS_CQ;
	cq_priv->cq_fid.fid.context = context;
	cq_priv->cq_fid.fid.ops = fi_cq_ops;
	cq_priv->cq_fid.ops = cq_ops;

	/*
	 * Although we don't need to store entry_size since we're already
	 * storing the format, this might provide a performance benefit
	 * when allocating storage.
	 */
	cq_priv->entry_size = format_sizes[cq_priv->attr.format];

	fastlock_init(&cq_priv->lock);
  ret = sstmac_cq_set_wait(cq_priv);
	if (ret)
		goto free_cq_priv;

  ret = _sstmac_queue_create(&cq_priv->events, alloc_cq_entry,
				 free_cq_entry, cq_priv->entry_size,
				 cq_priv->attr.size);
	if (ret)
		goto free_cq_priv;

  ret = _sstmac_queue_create(&cq_priv->errors, alloc_cq_entry,
				 free_cq_entry, sizeof(struct fi_cq_err_entry),
				 0);
	if (ret)
    goto free_sstmac_queue;

	*cq = &cq_priv->cq_fid;
	return ret;

free_sstmac_queue:
  _sstmac_queue_destroy(cq_priv->events);
free_cq_priv:
  _sstmac_ref_put(cq_priv->domain);
	fastlock_destroy(&cq_priv->lock);
	free(cq_priv);
free_fi_cq_ops:
	free(fi_cq_ops);
free_cq_ops:
	free(cq_ops);
#endif
	return ret;
}

static ssize_t __sstmacx_cq_readfrom(struct fid_cq *cq, void *buf,
					  size_t count, fi_addr_t *src_addr)
{
	ssize_t read_count = 0;
#if 0
	struct sstmacx_fid_cq *cq_priv;
	struct sstmacx_cq_entry *event;
	struct slist_entry *temp;


	if (!cq || !buf || !count)
		return -FI_EINVAL;

	cq_priv = container_of(cq, struct sstmacx_fid_cq, cq_fid);

	__sstmacx_cq_progress(cq_priv);

	if (_sstmacx_queue_peek(cq_priv->errors))
		return -FI_EAVAIL;

	COND_ACQUIRE(cq_priv->requires_lock, &cq_priv->lock);

	while (_sstmacx_queue_peek(cq_priv->events) && count--) {
		temp = _sstmacx_queue_dequeue(cq_priv->events);
		event = container_of(temp, struct sstmacx_cq_entry, item);

		assert(event->the_entry);
		memcpy(buf, event->the_entry, cq_priv->entry_size);
		if (src_addr)
			memcpy(&src_addr[read_count], &event->src_addr, sizeof(fi_addr_t));

		_sstmacx_queue_enqueue_free(cq_priv->events, &event->item);

		buf = (void *) ((uint8_t *) buf + cq_priv->entry_size);

		read_count++;
	}

	COND_RELEASE(cq_priv->requires_lock, &cq_priv->lock);
#endif
	return read_count ?: -FI_EAGAIN;
}

static ssize_t __sstmacx_cq_sreadfrom(int blocking, struct fid_cq *cq, void *buf,
				     size_t count, fi_addr_t *src_addr,
				     const void *cond, int timeout)
{
#if 0
	struct sstmacx_fid_cq *cq_priv;

	cq_priv = container_of(cq, struct sstmacx_fid_cq, cq_fid);
	if ((blocking && !cq_priv->wait) ||
	    (blocking && cq_priv->attr.wait_obj == FI_WAIT_SET))
		return -FI_EINVAL;

	if (_sstmacx_queue_peek(cq_priv->errors))
		return -FI_EAVAIL;

	if (cq_priv->wait)
		sstmacx_wait_wait((struct fid_wait *)cq_priv->wait, timeout);

#endif
	return __sstmacx_cq_readfrom(cq, buf, count, src_addr);

}

DIRECT_FN STATIC ssize_t sstmacx_cq_read(struct fid_cq *cq,
				      void *buf,
				      size_t count)
{
	return __sstmacx_cq_sreadfrom(0, cq, buf, count, NULL, NULL, 0);
}

DIRECT_FN STATIC ssize_t sstmacx_cq_sread(struct fid_cq *cq, void *buf,
				       size_t count, const void *cond,
				       int timeout)
{
	return __sstmacx_cq_sreadfrom(1, cq, buf, count, NULL, cond, timeout);
}

