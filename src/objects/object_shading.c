#include "objects_internal.h"

#define SET_SHIFT_AND_MASK(varShift, varMask, x) \
    varShift = x;                                \
    varMask = 0xFFFF >> x;

/**
 * Applies shading properties to a global variable.
 * Presumably intended for level geometry, which supports shading, but never uses it.
 */
void set_world_shading(f32 ambient, f32 diffuse, s16 angleX, s16 angleY, s16 angleZ) {
    set_shading_properties(&gWorldShading, ambient, diffuse, angleX, angleY, angleZ);
}

/**
 * Add values onto the existing properties of an objects shading.
 * Resets the shading based off the new values.
 */
UNUSED void add_shading_properties(Object *obj, f32 ambientChange, f32 diffuseChange, s16 angleX, s16 angleY,
                                   s16 angleZ) {
    if (obj->shading != NULL) {
        obj->shading->ambient += ambientChange;
        if (obj->shading->ambient < 0.0f) {
            obj->shading->ambient = 0.0f;
        } else if (obj->shading->ambient > 1.0f) {
            obj->shading->ambient = 1.0f;
        }
        obj->shading->diffuse += diffuseChange;
        if (obj->shading->diffuse < 0.0f) {
            obj->shading->diffuse = 0.0f;
        }
        if (obj->shading->diffuse >= 2.0f) {
            obj->shading->diffuse = 1.99f;
        }
        set_shading_properties(obj->shading, obj->shading->ambient, obj->shading->diffuse,
                               (obj->shading->shadeAngleX + angleX), (obj->shading->shadeAngleY + angleY),
                               (obj->shading->shadeAngleZ + angleZ));
        if (obj->header->shadeIntensityy != 0) {
            obj->shading->lightR = obj->header->shadeLightR;
            obj->shading->lightG = obj->header->shadeLightG;
            obj->shading->lightB = obj->header->shadeLightB;
            obj->shading->lightIntensity = obj->header->shadeIntensityy;
            obj->shading->lightDirX = -(obj->shading->shadowDirX >> 1);
            obj->shading->lightDirY = -(obj->shading->shadowDirY >> 1);
            obj->shading->lightDirZ = -(obj->shading->shadowDirZ >> 1);
        }
    }
}

void set_shading_properties(ShadeProperties *shading, f32 ambient, f32 diffuse, s16 angleX, s16 angleY, s16 angleZ) {
    Vec3s angle;
    Vec3f velocityPos;

    shading->shadeAngleX = angleX;
    shading->ambient = ambient;
    shading->diffuse = diffuse;
    shading->brightness = 1.0f;
    shading->shadeAngleY = angleY;
    shading->shadeAngleZ = angleZ;
    angle.z_rotation = angleX;
    angle.y_rotation = angleZ;
    angle.x_rotation = angleY;
    velocityPos.z = -16384.0f;
    velocityPos.x = 0.0f;
    velocityPos.y = 0.0f;
    vec3f_rotate(&angle, &velocityPos);
    shading->shadowDirX = -velocityPos.x;
    shading->shadowDirY = -velocityPos.y;
    shading->shadowDirZ = -velocityPos.z;
    shading->shadowR = 0;
    shading->shadowG = 0;
    shading->shadowB = 0;
}

/**
 * Take the normalised length of the position set by the perspective and set the world angle for the envmap.
 * Official name: setObjectViewNormal
 */
void update_envmap_position(f32 x, f32 y, f32 z) {
    f32 vecLength = sqrtf((x * x) + (y * y) + (z * z));
    f32 normalizedLength;
    if (vecLength != 0.0f) {
        normalizedLength = -8192.0f / vecLength;
        x *= normalizedLength;
        y *= normalizedLength;
        z *= normalizedLength;
    }
    gEnvmapPos[0].x = x;
    gEnvmapPos[0].y = y;
    gEnvmapPos[0].z = z;
}

/**
 * If the triangle batch allows for it, compute envmap normals for the mesh.
 * Some objects will prefer some extra additions on top before calculating, like light intensity.
 * cameraRelative turns the light through the projection matrix first; the HUD stopwatch passes FALSE.
 */
void obj_shade_fancy(ObjectModel *model, Object *object, s32 cameraRelative, f32 intensity) {
    s16 environmentMappingEnabled;
    s32 dynamicLightingEnabled;
    s16 i;

    dynamicLightingEnabled = 0;
    environmentMappingEnabled = 0;

    for (i = 0; i < model->numberOfBatches; i++) {
        if (model->batches[i].miscData != BATCH_VTX_COL) {
            dynamicLightingEnabled = -1; // This is a bit weird, but I guess it works.
        }
        if (model->batches[i].flags & RENDER_ENVMAP) {
            environmentMappingEnabled = -1;
        }
    }

    if (dynamicLightingEnabled) {
        // Calculates dynamic lighting for the object
        if (object->header->directionalPointLighting) {
            // Dynamic directional lighting for some objects (Intro diddy, Taj, T.T., Bosses)
            calc_dynamic_lighting_for_object_1(object, model, cameraRelative, object, intensity, 1.0f);
        } else {
            // Dynamic ambient lighting for other objects (Racers, Rare logo, Wizpig face, etc.)
            calc_dynamic_lighting_for_object_2(object, model, cameraRelative, intensity);
        }
    }

    if (environmentMappingEnabled) {
        // Calculates environment mapping for the object
        calc_env_mapping_for_object(model, object->trans.rotation.z_rotation, object->trans.rotation.x_rotation,
                                    object->trans.rotation.y_rotation);
    }
}

void calc_dynamic_lighting_for_object_1(Object *object, ObjectModel *model, s16 cameraRelative, Object *anotherObject,
                                        f32 intensity, f32 unusedScale) {
    s16 normIdx;
    s16 j;
    s16 i;
    Vec3s objRot;
    s32 lightStrength;
    s32 lightDirX, lightDirY, lightDirZ;    // 16.16 fixed point, normalized
    s32 shadowDirX, shadowDirY, shadowDirZ; // 16.16 fixed point, normalized
    s32 diffuseFactor;
    s32 ambientFactor;
    s32 lightIntensity;
    s32 shadeStrength;
    Vec3f direction;
    Vertex *vertices;
    Vec3s *normals;

    if (object->shading == NULL) {
        return;
    }

    vertices = object->curVertData;
    normals = model->normals;
    normIdx = 0;

    direction.x = -(object->shading->lightDirX << 3);
    direction.y = -(object->shading->lightDirY << 3);
    direction.z = -(object->shading->lightDirZ << 3);
    objRot.y_rotation = -object->trans.rotation.y_rotation;
    objRot.x_rotation = -object->trans.rotation.x_rotation;
    objRot.z_rotation = -object->trans.rotation.z_rotation;
    vec3f_rotate_ypr(&objRot, &direction);

    if (object->header->shadeIntensityy != 0 && cameraRelative) {
        mtxf_transform_dir(get_projection_matrix_f32(), &direction, &direction);
    }

    lightDirX = -direction.x;
    lightDirY = -direction.y;
    lightDirZ = -direction.z;
    lightStrength = object->shading->lightIntensity;

    direction.x = object->shading->shadowDirX << 2;
    direction.y = object->shading->shadowDirY << 2;
    direction.z = object->shading->shadowDirZ << 2;

    if (cameraRelative) {
        mtxf_transform_dir(get_projection_matrix_f32(), &direction, &direction);
    }
    vec3f_rotate_ypr(&objRot, &direction);

    shadowDirX = direction.x;
    shadowDirY = direction.y;
    shadowDirZ = direction.z;

    ambientFactor = object->shading->ambient * object->shading->brightness * 255.0f * intensity;
    diffuseFactor = object->shading->diffuse * object->shading->brightness * 255.0f * intensity;

    for (i = 0; i < model->numberOfBatches; i++) {
        if (model->batches[i].miscData != BATCH_VTX_COL) { // 0xFF means use vertex colors
            for (j = model->batches[i].verticesOffset; j < model->batches[i + 1].verticesOffset; j++) {
                // calculate lighting
                lightIntensity = (normals[normIdx].x * lightDirX + normals[normIdx].y * lightDirY +
                                  normals[normIdx].z * lightDirZ) >>
                                 13;
                if (lightIntensity > 0) {
                    lightIntensity = (lightIntensity * lightStrength) >> 16;
                    if (lightIntensity > 255) {
                        lightIntensity = 255;
                    }
                } else {
                    lightIntensity = 0;
                }

                // calculate shading
                shadeStrength = (normals[normIdx].x * shadowDirX + normals[normIdx].y * shadowDirY +
                                 normals[normIdx].z * shadowDirZ) >>
                                13;
                if (shadeStrength > 0) {
                    shadeStrength = (shadeStrength * diffuseFactor) >> 16;
                    shadeStrength += ambientFactor;
                    if (shadeStrength > 255) {
                        shadeStrength = 255;
                    }
                } else {
                    shadeStrength = ambientFactor;
                }

                vertices[j].r = lightIntensity;
                vertices[j].g = lightIntensity;
                vertices[j].b = lightIntensity;
                vertices[j].a = shadeStrength;
                normIdx++;
            }
        } else if (model->batches[i].flags & RENDER_ENVMAP) {
            normIdx += model->batches[i + 1].verticesOffset - model->batches[i].verticesOffset;
        }
    }
}

void calc_env_mapping_for_object(ObjectModel *model, s16 zRot, s16 xRot, s16 yRot) {
    MtxS objRotMtxS32;
    MtxF objRotMtxF32;
    ObjectTransform objTrans;
    s16 k;
    s16 count;
    Triangle *triangles;
    Vec3s *normals;
    s32 skipReflect;
    TextureHeader *tex;
    s16 shiftS;
    s16 maskS;
    s16 shiftT;
    s16 maskT;
    s16 i;
    s16 j;
    s16 texU;
    s16 texV;

    count = 0;
    triangles = model->triangles;
    normals = model->normals;
    objTrans.rotation.z_rotation = zRot;
    objTrans.rotation.x_rotation = xRot;
    objTrans.rotation.y_rotation = yRot;
    objTrans.x_position = 0.0f;
    objTrans.y_position = 0.0f;
    objTrans.z_position = 0.0f;
    objTrans.scale = 1.0f;
    mtxf_from_transform(&objRotMtxF32, &objTrans);
    mtxf_to_mtxs(&objRotMtxF32, &objRotMtxS32);
    for (i = 0; i < model->numberOfBatches; i++) {
        if (model->batches[i].flags & RENDER_ENVMAP) {
            skipReflect = ((model->batches[i].flags & RENDER_UNK_0020000) | RENDER_ENVMAP) ^ RENDER_ENVMAP;
            tex = model->textures[model->batches[i].textureIndex].texture;
            k = 0;

            switch (tex->width) {
                case 0x80:
                    SET_SHIFT_AND_MASK(shiftS, maskS, 4);
                    break;
                case 0x40:
                    SET_SHIFT_AND_MASK(shiftS, maskS, 5);
                    break;
                case 0x20:
                    SET_SHIFT_AND_MASK(shiftS, maskS, 6);
                    break;
                default:
                    SET_SHIFT_AND_MASK(shiftS, maskS, 7);
                    break;
            }
            switch (tex->height) {
                case 0x80:
                    SET_SHIFT_AND_MASK(shiftT, maskT, 4);
                    break;
                case 0x40:
                    SET_SHIFT_AND_MASK(shiftT, maskT, 5);
                    break;
                case 0x20:
                    SET_SHIFT_AND_MASK(shiftT, maskT, 6);
                    break;
                default:
                    SET_SHIFT_AND_MASK(shiftT, maskT, 7);
                    break;
            }

            for (j = model->batches[i].verticesOffset; j < model->batches[i + 1].verticesOffset; j++, k++) {
                gEnvmapPos[1].x = normals[count].x;
                gEnvmapPos[1].y = normals[count].y;
                gEnvmapPos[1].z = normals[count].z;
                mtxs_transform_dir(&objRotMtxS32, &gEnvmapPos[1]);
                if (skipReflect == 0) {
                    vec3s_reflect(&gEnvmapPos[0], &gEnvmapPos[1]);
                }
                count++;
                texU = gEnvmapPos[1].x;
                texV = gEnvmapPos[1].y;
                if (texU > 0) {
                    texU--;
                }
                if (texV > 0) {
                    texV--;
                }
                texU = (texU << 2) + 0x8000;
                texV = (texV << 2) + 0x8000;
                gTexCoordScratch[k].u = (texU >> shiftS) & maskS;
                gTexCoordScratch[k].v = (texV >> shiftT) & maskT;
            }

            for (j = model->batches[i].facesOffset; j < model->batches[i + 1].facesOffset; j++) {
                triangles[j].uv0.u = gTexCoordScratch[triangles[j].vi0].u;
                triangles[j].uv0.v = gTexCoordScratch[triangles[j].vi0].v;
                triangles[j].uv1.u = gTexCoordScratch[triangles[j].vi1].u;
                triangles[j].uv1.v = gTexCoordScratch[triangles[j].vi1].v;
                triangles[j].uv2.u = gTexCoordScratch[triangles[j].vi2].u;
                triangles[j].uv2.v = gTexCoordScratch[triangles[j].vi2].v;
            }
        } else if (model->batches[i].miscData < BATCH_VTX_COL) {
            count += model->batches[i + 1].verticesOffset - model->batches[i].verticesOffset;
        }
    }
}
