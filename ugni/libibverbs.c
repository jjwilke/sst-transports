#include "verbs.h"

const char *ibv_get_device_name(struct ibv_device *device){
  return "a device";
}

struct ibv_xrc_domain *ibv_open_xrc_domain(struct ibv_context *context, int fd, int oflag){
  return 0;
}

int ibv_cmd_open_xrcd(struct ibv_context *context, struct verbs_xrcd *xrcd,
  int vxrcd_size,
  struct ibv_xrcd_init_attr *attr,
  struct ibv_open_xrcd *cmd, size_t cmd_size,
  struct ibv_open_xrcd_resp *resp, size_t resp_size){
  return 0;
}

