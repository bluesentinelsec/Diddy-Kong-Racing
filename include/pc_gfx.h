#ifndef PC_GFX_H
#define PC_GFX_H

/**
 * @file pc_gfx.h
 * What the game tells the host renderer (linux/main.c) beyond the display list.
 *
 * pc_gfx_mtx_tag() names the matrix the game just pushed: which object,
 * camera or sprite it positions, and from which code path. The host uses it to
 * find the same matrix in the previous frame and interpolate between the two
 * when it presents frames in between logic ticks. A matrix without a tag is
 * drawn where it is; the display list itself is untouched.
 *
 * For the PERSPECTIVE, ORIGIN, MODEL, SHEAR and BILLBOARD sites the key must
 * point at the ObjectTransform the matrix was built from (a Camera, an Object
 * or the transform itself — all start with one): the host snapshots it to tell
 * a camera cut or a teleport from motion, and snaps instead of interpolating.
 */

enum PcMtxSite {
    PC_MTX_SITE_PERSPECTIVE = 1, // mtx_perspective: the camera's view-projection
    PC_MTX_SITE_ORTHO,           // mtx_ortho
    PC_MTX_SITE_ORIGIN,          // mtx_world_origin
    PC_MTX_SITE_MODEL,           // mtx_cam_push: an object transform
    PC_MTX_SITE_SHEAR,           // mtx_shear_push
    PC_MTX_SITE_HEAD,            // mtx_head_push: a racer's head
    PC_MTX_SITE_BILLBOARD,       // render_sprite_billboard
    PC_MTX_SITE_ORTHO_IMAGE,     // render_ortho_triangle_image
    PC_MTX_SITE_WEATHER,         // weather.c snow
    PC_MTX_SITE_WATER            // waves.c: one water tile (or quadrant), re-tagged over its MODEL push
};

void pc_gfx_mtx_tag(const void *mtx, const void *key, int site);

#endif
