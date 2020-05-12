#ifndef sstmac_fake_verbs_h
#define sstmac_fake_verbs_h

#include <stddef.h>

enum ibv_transport_type {
  IBV_TRANSPORT_UNKNOWN,
  IBV_TRANSPORT_IB,
  IBV_TRANSPORT_IWARP,
};

enum ibv_node_type {
  IBV_NODE_UNKNOWN,
  IBV_NODE_CA,
  IBV_NODE_SWITCH,
  IBV_NODE_ROUTER,
  IBV_NODE_RNIC
};


#define IBV_SYSFS_NAME_MAX 100
#define IBV_SYSFS_PATH_MAX 100
struct ibv_device {
  enum ibv_transport_type transport_type;
  enum ibv_node_type node_type;
  char  dev_name [IBV_SYSFS_NAME_MAX];
  char  dev_path [IBV_SYSFS_PATH_MAX];
  char  ibdev_path [IBV_SYSFS_PATH_MAX];
  char  name [IBV_SYSFS_NAME_MAX];
};

#ifdef __cplusplus
extern "C" {
#endif

const char *ibv_get_device_name(struct ibv_device *device);

struct ibv_xrc_domain {
  int member;
};

struct ibv_context {
  int member;
};

struct ibv_xrc_domain *ibv_open_xrc_domain(struct ibv_context *context, int fd, int oflag);

struct verbs_xrcd {
  int member;
};

struct ibv_xrcd_init_attr {
  int member;
};

struct ibv_open_xrcd {
  int member;
};

struct ibv_open_xrcd_resp {
  int member;
};

int ibv_cmd_open_xrcd(struct ibv_context *context, struct verbs_xrcd *xrcd,
  int vxrcd_size,
  struct ibv_xrcd_init_attr *attr,
  struct ibv_open_xrcd *cmd, size_t cmd_size,
  struct ibv_open_xrcd_resp *resp, size_t resp_size);


#ifdef __cplusplus
}
#endif

#endif
