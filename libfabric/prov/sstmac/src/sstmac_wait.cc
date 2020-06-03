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
#include <stdlib.h>
#include <signal.h>
#include "sstmac.h"
#include "sstmac_wait.h"

static int sstmac_wait_control(struct fid *wait, int command, void *arg);
extern "C" int sstmac_wait_close(struct fid *wait);
DIRECT_FN extern "C" int sstmac_wait_wait(struct fid_wait *wait, int timeout);
DIRECT_FN extern "C" int sstmac_wait_open(struct fid_fabric *fabric,
			     struct fi_wait_attr *attr,
			     struct fid_wait **waitset);

static struct fi_ops sstmac_fi_ops = {
	.size = sizeof(struct fi_ops),
	.close = sstmac_wait_close,
	.bind = fi_no_bind,
	.control = sstmac_wait_control,
	.ops_open = fi_no_ops_open
};

static struct fi_ops_wait sstmac_wait_ops = {
	.size = sizeof(struct fi_ops_wait),
	.wait = sstmac_wait_wait
};

static int sstmac_wait_control(struct fid *wait, int command, void *arg)
{
/*
	struct fid_wait *wait_fid_priv;

	SSTMAC_TRACE(WAIT_SUB, "\n");

	wait_fid_priv = container_of(wait, struct fid_wait, fid);
*/

	switch (command) {
	case FI_GETWAIT:
		return -FI_ENOSYS;
	default:
		return -FI_EINVAL;
	}
}

DIRECT_FN extern "C" int sstmac_wait_wait(struct fid_wait *wait, int timeout)
{
	int err = 0, ret;
	char c;
#if 0
	struct sstmac_fid_wait *wait_priv;

	SSTMAC_TRACE(WAIT_SUB, "\n");

	wait_priv = container_of(wait, struct sstmac_fid_wait, wait.fid);
	switch (wait_priv->type) {
	case FI_WAIT_UNSPEC:
		pthread_mutex_lock(&sstmac_wait_mutex);
		sstmac_wait_thread_enabled++;
		pthread_cond_signal(&sstmac_wait_cond);
		pthread_mutex_unlock(&sstmac_wait_mutex);
		SSTMAC_DEBUG(WAIT_SUB,
			   "Calling fi_poll_fd %d timeout %d\n",
			   wait_priv->fd[WAIT_READ],
			   timeout);
		err = fi_poll_fd(wait_priv->fd[WAIT_READ], timeout);
		SSTMAC_DEBUG(WAIT_SUB, "Return code from poll was %d\n", err);
		if (err == 0) {
			err = -FI_ETIMEDOUT;
		} else {
			while (err > 0) {
				ret = ofi_read_socket(wait_priv->fd[WAIT_READ],
						      &c,
						      1);
				SSTMAC_DEBUG(WAIT_SUB, "ret is %d C is %c\n",
					  ret,
					  c);
				if (ret != 1) {
					SSTMAC_ERR(WAIT_SUB,
						 "failed to read wait_fd\n");
					err = 0;
					break;
				}
				err--;
			}
		}
		break;
	default:
		SSTMAC_WARN(WAIT_SUB, "Invalid wait object type\n");
		return -FI_EINVAL;
	}
	pthread_mutex_lock(&sstmac_wait_mutex);
	sstmac_wait_thread_enabled--;
	pthread_mutex_unlock(&sstmac_wait_mutex);
#endif
	return err;
}

extern "C" int sstmac_wait_close(struct fid *wait)
{
#if 0
	struct sstmac_fid_wait *wait_priv;

	SSTMAC_TRACE(WAIT_SUB, "\n");

	wait_priv = container_of(wait, struct sstmac_fid_wait, wait.fid);

	if (!slist_empty(&wait_priv->set)) {
		SSTMAC_WARN(WAIT_SUB,
			  "resources still connected to wait set.\n");
		return -FI_EBUSY;
	}

	if (wait_priv->type == FI_WAIT_FD) {
		close(wait_priv->fd[WAIT_READ]);
		close(wait_priv->fd[WAIT_WRITE]);
	}

	_sstmac_ref_put(wait_priv->fabric);

	free(wait_priv);

	__sstmac_wait_stop_progress();
#endif
	return FI_SUCCESS;
}

DIRECT_FN int sstmac_wait_open(struct fid_fabric *fabric,
			     struct fi_wait_attr *attr,
			     struct fid_wait **waitset)
{
	int ret = FI_SUCCESS;
#if 0
	struct sstmac_fid_fabric *fab_priv;
	struct sstmac_fid_wait *wait_priv;

	SSTMAC_TRACE(WAIT_SUB, "\n");

	ret = sstmac_verify_wait_attr(attr);
	if (ret)
		goto err;

	fab_priv = container_of(fabric, struct sstmac_fid_fabric, fab_fid);

	wait_priv = calloc(1, sizeof(*wait_priv));
	if (!wait_priv) {
		SSTMAC_WARN(WAIT_SUB,
			 "failed to allocate memory for wait set.\n");
		ret = -FI_ENOMEM;
		goto err;
	}

	ret = sstmac_init_wait_obj(wait_priv, attr->wait_obj);
	if (ret)
		goto cleanup;

	slist_init(&wait_priv->set);

	wait_priv->wait.fid.fclass = FI_CLASS_WAIT;
	wait_priv->wait.fid.ops = &sstmac_fi_ops;
	wait_priv->wait.ops = &sstmac_wait_ops;

	wait_priv->fabric = fab_priv;

	_sstmac_ref_get(fab_priv);
	*waitset = &wait_priv->wait;

	__sstmac_wait_start_progress();
	return ret;

cleanup:
	free(wait_priv);
err:
#endif
	return ret;
}

