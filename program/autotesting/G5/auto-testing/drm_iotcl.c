#include "svapi.h"
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <drm.h>
#include <drm_mode.h>
#include <omap_drm.h>
#include <omap_drmif.h>
#include <xf86drmMode.h>
#include <errno.h>

#define DRM_IOCTL_MODE_GETVIDPLANERESOURCES DRM_IOWR(0xBC, struct drm_mode_get_plane_res)
#define U642VOID(x) ((void *)(unsigned long)(x))
#define VOID2U64(x) ((uint64_t)(unsigned long)(x))

static void *local_drmMalloc(int size)
{
    void *pt;
    if ((pt = malloc(size)))
	memset(pt, 0, size);
    return pt;
}

static void local_drmFree(void *pt)
{
    if (pt)
	free(pt);
}
/*
 * Util functions
 */

void* local_drmAllocCpy(void *array, int count, int entry_size)
{
	char *r;
	int i;

	if (!count || !array || !entry_size)
		return 0;

	if (!(r = local_drmMalloc(count*entry_size)))
		return 0;

	for (i = 0; i < count; i++)
		memcpy(r+(entry_size*i), array+(entry_size*i), entry_size);

	return r;
}
/**
 * Call ioctl, restarting if it is interupted
 */
static int
local_drmIoctl(int fd, unsigned long request, void *arg)
{
    int	ret;

    do {
	ret = ioctl(fd, request, arg);
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));
    return ret;
}

static inline int local_DRM_IOCTL(int fd, unsigned long cmd, void *arg)
{
	int ret = local_drmIoctl(fd, cmd, arg);
	return ret < 0 ? -errno : ret;
}
static int local_drmGetMagic(int fd, drm_magic_t * magic)
{
    drm_auth_t auth;

    *magic = 0;
    if (local_drmIoctl(fd, DRM_IOCTL_GET_MAGIC, &auth))
	return -errno;
    *magic = auth.magic;
    return 0;
}

static int local_drmAuthMagic(int fd, drm_magic_t magic)
{
    drm_auth_t auth;

    auth.magic = magic;
    if (local_drmIoctl(fd, DRM_IOCTL_AUTH_MAGIC, &auth))
	return -errno;
    return 0;
}

static int local_drmDropMaster(int fd)
{
	return ioctl(fd, DRM_IOCTL_DROP_MASTER, 0);
}

static inline int local_is_drm_master(int drm_fd)
{
	drm_magic_t magic;

	return local_drmGetMagic(drm_fd, &magic) == 0 &&
		local_drmAuthMagic(drm_fd, magic) == 0;
}
/*
 * ModeSetting functions.
 */

static drmModeResPtr local_drmModeGetResources(int fd)
{
	struct drm_mode_card_res res, counts;
	drmModeResPtr r = 0;

retry:
	memset(&res, 0, sizeof(struct drm_mode_card_res));
	if (local_drmIoctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
		return 0;

	counts = res;

	if (res.count_fbs) {
		res.fb_id_ptr = VOID2U64(local_drmMalloc(res.count_fbs*sizeof(uint32_t)));
		if (!res.fb_id_ptr)
			goto err_allocs;
	}
	if (res.count_crtcs) {
		res.crtc_id_ptr = VOID2U64(local_drmMalloc(res.count_crtcs*sizeof(uint32_t)));
		if (!res.crtc_id_ptr)
			goto err_allocs;
	}
	if (res.count_connectors) {
		res.connector_id_ptr = VOID2U64(local_drmMalloc(res.count_connectors*sizeof(uint32_t)));
		if (!res.connector_id_ptr)
			goto err_allocs;
	}
	if (res.count_encoders) {
		res.encoder_id_ptr = VOID2U64(local_drmMalloc(res.count_encoders*sizeof(uint32_t)));
		if (!res.encoder_id_ptr)
			goto err_allocs;
	}

	if (local_drmIoctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
		goto err_allocs;

	/* The number of available connectors and etc may have changed with a
	 * hotplug event in between the ioctls, in which case the field is
	 * silently ignored by the kernel.
	 */
	if (counts.count_fbs < res.count_fbs ||
	    counts.count_crtcs < res.count_crtcs ||
	    counts.count_connectors < res.count_connectors ||
	    counts.count_encoders < res.count_encoders)
	{
		local_drmFree(U642VOID(res.fb_id_ptr));
		local_drmFree(U642VOID(res.crtc_id_ptr));
		local_drmFree(U642VOID(res.connector_id_ptr));
		local_drmFree(U642VOID(res.encoder_id_ptr));

		goto retry;
	}

	/*
	 * return
	 */
	if (!(r = local_drmMalloc(sizeof(*r))))
		goto err_allocs;

	r->min_width     = res.min_width;
	r->max_width     = res.max_width;
	r->min_height    = res.min_height;
	r->max_height    = res.max_height;
	r->count_fbs     = res.count_fbs;
	r->count_crtcs   = res.count_crtcs;
	r->count_connectors = res.count_connectors;
	r->count_encoders = res.count_encoders;

	r->fbs        = local_drmAllocCpy(U642VOID(res.fb_id_ptr), res.count_fbs, sizeof(uint32_t));
	r->crtcs      = local_drmAllocCpy(U642VOID(res.crtc_id_ptr), res.count_crtcs, sizeof(uint32_t));
	r->connectors = local_drmAllocCpy(U642VOID(res.connector_id_ptr), res.count_connectors, sizeof(uint32_t));
	r->encoders   = local_drmAllocCpy(U642VOID(res.encoder_id_ptr), res.count_encoders, sizeof(uint32_t));
	if ((res.count_fbs && !r->fbs) ||
	    (res.count_crtcs && !r->crtcs) ||
	    (res.count_connectors && !r->connectors) ||
	    (res.count_encoders && !r->encoders))
	{
		local_drmFree(r->fbs);
		local_drmFree(r->crtcs);
		local_drmFree(r->connectors);
		local_drmFree(r->encoders);
		local_drmFree(r);
		r = 0;
	}

err_allocs:
	local_drmFree(U642VOID(res.fb_id_ptr));
	local_drmFree(U642VOID(res.crtc_id_ptr));
	local_drmFree(U642VOID(res.connector_id_ptr));
	local_drmFree(U642VOID(res.encoder_id_ptr));

	return r;
}

/*
 * Crtc functions
 */

static drmModeCrtcPtr local_drmModeGetCrtc(int fd, uint32_t crtcId)
{
	struct drm_mode_crtc crtc;
	drmModeCrtcPtr r;

	crtc.crtc_id = crtcId;

	if (local_drmIoctl(fd,DRM_IOCTL_MODE_GETCRTC, &crtc))
		return 0;

	/*
	 * return
	 */

	if (!(r = local_drmMalloc(sizeof(*r))))
		return 0;

	r->crtc_id         = crtc.crtc_id;
	r->x               = crtc.x;
	r->y               = crtc.y;
	r->mode_valid      = crtc.mode_valid;
	if (r->mode_valid) {
		memcpy(&r->mode, &crtc.mode, sizeof(struct drm_mode_modeinfo));
		r->width = crtc.mode.hdisplay;
		r->height = crtc.mode.vdisplay;
	}
	r->buffer_id       = crtc.fb_id;
	r->gamma_size      = crtc.gamma_size;
	return r;
}

static drmModeObjectPropertiesPtr local_drmModeObjectGetProperties(int fd,
						      uint32_t object_id,
						      uint32_t object_type)
{
	struct drm_mode_obj_get_properties properties;
	drmModeObjectPropertiesPtr ret = NULL;
	uint32_t count;

retry:
	memset(&properties, 0, sizeof(struct drm_mode_obj_get_properties));
	properties.obj_id = object_id;
	properties.obj_type = object_type;

	if (local_drmIoctl(fd, DRM_IOCTL_MODE_OBJ_GETPROPERTIES, &properties))
		return 0;

	count = properties.count_props;

	if (count) {
		properties.props_ptr = VOID2U64(local_drmMalloc(count *
							  sizeof(uint32_t)));
		if (!properties.props_ptr)
			goto err_allocs;
		properties.prop_values_ptr = VOID2U64(local_drmMalloc(count *
						      sizeof(uint64_t)));
		if (!properties.prop_values_ptr)
			goto err_allocs;
	}

	if (local_drmIoctl(fd, DRM_IOCTL_MODE_OBJ_GETPROPERTIES, &properties))
		goto err_allocs;

	if (count < properties.count_props) {
		local_drmFree(U642VOID(properties.props_ptr));
		local_drmFree(U642VOID(properties.prop_values_ptr));
		goto retry;
	}
	count = properties.count_props;

	ret = local_drmMalloc(sizeof(*ret));
	if (!ret)
		goto err_allocs;

	ret->count_props = count;
	ret->props = local_drmAllocCpy(U642VOID(properties.props_ptr),
				 count, sizeof(uint32_t));
	ret->prop_values = local_drmAllocCpy(U642VOID(properties.prop_values_ptr),
				       count, sizeof(uint64_t));
	if (ret->count_props && (!ret->props || !ret->prop_values)) {
		local_drmFree(ret->props);
		local_drmFree(ret->prop_values);
		local_drmFree(ret);
		ret = NULL;
	}

err_allocs:
	local_drmFree(U642VOID(properties.props_ptr));
	local_drmFree(U642VOID(properties.prop_values_ptr));
	return ret;
}

static int local_drmModeObjectSetProperty(int fd, uint32_t object_id, uint32_t object_type,
			     uint32_t property_id, uint64_t value)
{
	struct drm_mode_obj_set_property prop;

	prop.value = value;
	prop.prop_id = property_id;
	prop.obj_id = object_id;
	prop.obj_type = object_type;

	return local_DRM_IOCTL(fd, DRM_IOCTL_MODE_OBJ_SETPROPERTY, &prop);
}

static void local_drmModeFreeObjectProperties(drmModeObjectPropertiesPtr ptr)
{
	if (!ptr)
		return;
	local_drmFree(ptr->props);
	local_drmFree(ptr->prop_values);
	local_drmFree(ptr);
}

static void local_drmModeFreeCrtc(drmModeCrtcPtr ptr)
{
	if (!ptr)
		return;

	local_drmFree(ptr);

}

static void local_drmModeFreeResources(drmModeResPtr ptr)
{
	if (!ptr)
		return;

	local_drmFree(ptr->fbs);
	local_drmFree(ptr->crtcs);
	local_drmFree(ptr->connectors);
	local_drmFree(ptr->encoders);
	local_drmFree(ptr);

}

static drmModePlaneResPtr drmModeGetVidPlaneResources(int fd)
{
	struct drm_mode_get_plane_res res, counts;
	drmModePlaneResPtr r = 0;

retry:
	memset(&res, 0, sizeof(struct drm_mode_get_plane_res));
	if (local_drmIoctl(fd, DRM_IOCTL_MODE_GETVIDPLANERESOURCES, &res))
		return 0;

	counts = res;

	if (res.count_planes) {
		res.plane_id_ptr = VOID2U64(local_drmMalloc(res.count_planes *
							sizeof(uint32_t)));
		if (!res.plane_id_ptr)
			goto err_allocs;
	}

	if (local_drmIoctl(fd, DRM_IOCTL_MODE_GETVIDPLANERESOURCES, &res))
		goto err_allocs;

	if (counts.count_planes < res.count_planes) {
		local_drmFree(U642VOID(res.plane_id_ptr));
		goto retry;
	}

	if (!(r = local_drmMalloc(sizeof(*r))))
		goto err_allocs;

	r->count_planes = res.count_planes;
	r->planes = (uint32_t*)local_drmAllocCpy(U642VOID(res.plane_id_ptr),
				  res.count_planes, sizeof(uint32_t));
	if (res.count_planes && !r->planes) {
		local_drmFree(r->planes);
		local_drmFree(r);
		r = 0;
	}

err_allocs:
	local_drmFree(U642VOID(res.plane_id_ptr));

	return r;
}

static void local_drmModeFreePlaneResources(drmModePlaneResPtr ptr)
{
	if (!ptr)
		return;

	local_drmFree(ptr->planes);
	local_drmFree(ptr);
}

static drmModePlanePtr local_drmModeGetPlane(int fd, uint32_t plane_id)
{
	struct drm_mode_get_plane ovr, counts;
	drmModePlanePtr r = 0;

retry:
	memset(&ovr, 0, sizeof(struct drm_mode_get_plane));
	ovr.plane_id = plane_id;
	if (local_drmIoctl(fd, DRM_IOCTL_MODE_GETPLANE, &ovr))
		return 0;

	counts = ovr;

	if (ovr.count_format_types) {
		ovr.format_type_ptr = VOID2U64(local_drmMalloc(ovr.count_format_types *
							 sizeof(uint32_t)));
		if (!ovr.format_type_ptr)
			goto err_allocs;
	}

	if (local_drmIoctl(fd, DRM_IOCTL_MODE_GETPLANE, &ovr))
		goto err_allocs;

	if (counts.count_format_types < ovr.count_format_types) {
		local_drmFree(U642VOID(ovr.format_type_ptr));
		goto retry;
	}

	if (!(r = local_drmMalloc(sizeof(*r))))
		goto err_allocs;

	r->count_formats = ovr.count_format_types;
	r->plane_id = ovr.plane_id;
	r->crtc_id = ovr.crtc_id;
	r->fb_id = ovr.fb_id;
	r->possible_crtcs = ovr.possible_crtcs;
	r->gamma_size = ovr.gamma_size;
	r->formats = local_drmAllocCpy(U642VOID(ovr.format_type_ptr),
				 ovr.count_format_types, sizeof(uint32_t));
	if (ovr.count_format_types && !r->formats) {
		local_drmFree(r->formats);
		local_drmFree(r);
		r = 0;
	}

err_allocs:
	local_drmFree(U642VOID(ovr.format_type_ptr));

	return r;
}

static void local_drmModeFreePlane(drmModePlanePtr ptr)
{
	if (!ptr)
		return;

	local_drmFree(ptr->formats);
	local_drmFree(ptr);
}

int drm_Setplane_Property(CMD_VID_IF *cmd_vid_if)
{
	drmModePlaneRes *plane_resources;
	drmModePlane *vidplane;
	int fd = -1,ret = 0;
	if(cmd_vid_if == NULL) {
		fprintf(stderr, "OS:svapi:drm_Setplane_Property param is null\n");
		return -1;
	}

	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		printf("OS:svapi:open /dev/dri/card0 failed.\n");
		return -1;
	}

	if(local_is_drm_master(fd)) {
		local_drmDropMaster(fd);
	}

	plane_resources = drmModeGetVidPlaneResources(fd);
	if (!plane_resources) {
		fprintf(stderr, "OS:svapi:drmModeGetVidPlaneResources failed: %s\n",strerror(errno));
		close(fd);
		return -1;
	}

	if(plane_resources->count_planes < cmd_vid_if->index) {
		fprintf(stderr, "OS:svapi:Plane num not enough: %d.\n",plane_resources->count_planes);
	}

	vidplane = local_drmModeGetPlane(fd, plane_resources->planes[cmd_vid_if->index]);
	if (!vidplane) {
		fprintf(stderr, "OS:svapi:drmModeGetPlane failed: %s\n",strerror(errno));
		return -1;
	}

	ret = local_drmModeObjectSetProperty(fd,vidplane->plane_id, DRM_MODE_OBJECT_PLANE, cmd_vid_if->prop_id, cmd_vid_if->prop_value);
	if (ret < 0) {
		printf("OS:svapi:Could not SetPropertyr for plane\n");
	}

	local_drmModeFreePlane(vidplane);
	local_drmModeFreePlaneResources(plane_resources);
	close(fd);
	return ret;
}

int drm_Setcrtc_Property(CMD_GFX_IF *cmd_gfx_if)
{
	drmModeRes *resources;
	drmModeCrtc *crtc;
	drmModeObjectPropertiesPtr props;
	drmModePropertyPtr prop;
	int i,j,fd = -1,ret = 0;
	if(cmd_gfx_if == NULL) {
		printf("OS:svapi:drm_Setcrtc_Property params error.\n");
		return -1;
	}

	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		printf("OS:svapi:open /dev/dri/card0 failed.\n");
		return -1;
	}

	if(local_is_drm_master(fd)) {
		local_drmDropMaster(fd);
	}

	resources = local_drmModeGetResources(fd);
	if (!resources) {
		fprintf(stderr, "OS:svapi:local_drmModeGetResources failed: %s\n",strerror(errno));
		close(fd);
		return -1;
	}

	/* G5 only one crtc */
	for (i = 0; i < resources->count_crtcs; i++) {
		crtc = local_drmModeGetCrtc(fd, resources->crtcs[i]);

		if (!crtc) {
			fprintf(stderr, "OS:svapi:could not get crtc %i: %s\n",
				resources->crtcs[i], strerror(errno));
			continue;
		}

		props = local_drmModeObjectGetProperties(fd, crtc->crtc_id,DRM_MODE_OBJECT_CRTC);
		if (props) {
			for (j = 0; j < props->count_props; j++) {
				if(props->props[j] == cmd_gfx_if->prop_id) {
					ret = local_drmModeObjectSetProperty(fd,crtc->crtc_id,
					DRM_MODE_OBJECT_CRTC,cmd_gfx_if->prop_id,cmd_gfx_if->prop_value);
					if(ret < 0) {
							fprintf(stderr, "OS:svapi:could not set crtc prop:%d: %s\n",
							cmd_gfx_if->prop_id,strerror(errno));
						}
					break;
				} else {
					continue;
				}
			}
			if(j == props->count_props) {
				printf("\tOS:svapi:can not match prop id:%d.\n",cmd_gfx_if->prop_id);
			}
			local_drmModeFreeObjectProperties(props);
		} else {
			printf("\tOS:svapi:could not get crtc properties: %s\n",strerror(errno));
		}
		local_drmModeFreeCrtc(crtc);
	}
	local_drmModeFreeResources(resources);
	close(fd);
	return 0;
}
