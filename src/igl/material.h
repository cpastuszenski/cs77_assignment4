#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "node.h"
#include "texture.h"
#include "vmath/montecarlo.h"

///@file igl/material.h Materials. @ingroup igl
///@defgroup material Materials
///@ingroup igl
///@{

/// Abstract Material
struct Material : Node {
    REGISTER_FAST_RTTI(Node,Material,8)
    
    Texture*     normal_texture = nullptr; ///< normal map
};

/// Lambert Material
struct Lambert : Material {
    REGISTER_FAST_RTTI(Material,Lambert,1)
    
    vec3f        diffuse = vec3f(0.75,0.75,0.75); ///< diffuse color
    Texture*     diffuse_texture = nullptr; ///< diffuse texture
};

/// Phong Material
struct Phong : Material {
    REGISTER_FAST_RTTI(Material,Phong,2)
    
	vec3f        diffuse = vec3f(0.75,0.75,0.75); ///< diffuse color
    vec3f        specular = vec3f(0.25,0.25,0.25); ///< specular color
    float        exponent =  10; ///< specular exponent
    vec3f        reflection = zero3f; ///< reflection color
    float        blur_size = 0.0f; ///< blurriness of reflection
    bool         texture_mapping = true; ///< whether we are going to use texture mapping
    bool         trilinear = false; ///< whether we are going to use trilinear filtering techniques
    Texture*     diffuse_texture = nullptr; ///< diffuse texture
    Texture*     diffuse_texture_tri_1 = nullptr;
    Texture*     diffuse_texture_tri_2 = nullptr;
    Texture*     diffuse_texture_tri_3 = nullptr;
    Texture*     specular_texture = nullptr; ///< specular texture
    Texture*     exponent_texture = nullptr; ///< specular exponent texture
    Texture*     reflection_texture = nullptr; ///< reflection texture
    
    bool use_reflected = false; ///< use reflected or bisector
};

/// Lambert Emission Material
struct LambertEmission : Material {
    REGISTER_FAST_RTTI(Material,LambertEmission,5)
    
    vec3f       emission = one3f; ///< diffuse emission color
    vec3f       diffuse = one3f; ///< lambert diffuse coefficient
    Texture*    emission_texture = nullptr; ///< emission texture
    Texture*    diffuse_texture = nullptr; ///< emission texture
};

///@name eval interface
///@{

/// check whether a material has textures
inline bool material_has_textures(Material* material) {
    if(is<Lambert>(material)) {
        auto lambert = cast<Lambert>(material);
        return lambert->diffuse_texture;
    }
    else if(is<Phong>(material)) {
        auto phong = cast<Phong>(material);
        return phong->diffuse_texture or phong->specular_texture or phong->exponent_texture or phong->reflection_texture;
    }
    else if(is<LambertEmission>(material)) {
        auto emission = cast<LambertEmission>(material);
        return emission->diffuse_texture or emission->emission_texture;
    }
    else { NOT_IMPLEMENTED_ERROR(); return nullptr; }
}

/// evalute perturbed shading frame
inline frame3f material_shading_frame(Material* material, const frame3f& frame, const vec2f& texcoord) {
    return frame;
}

/// resolve texture coordinates
inline Material* material_shading_textures(Material* material, const vec2f& texcoord, float dist) {
    if(is<Lambert>(material)) {
        auto lambert = cast<Lambert>(material);
        auto ret = new Lambert();
        ret->diffuse = lambert->diffuse;
        return ret;
    }
    else if(is<Phong>(material)) {
        auto phong = cast<Phong>(material);
        auto ret = new Phong();
        ret->blur_size = phong->blur_size;
        ret->use_reflected = phong->use_reflected;
        ret->diffuse = phong->diffuse;
        // Flag to use texture mapping
        if(phong->texture_mapping) {
            // Texture filtering (5 Points)
            // This flag decides whether we consider using mipmaps/smaller textures (64 times as patterned as normal)
            // This is done in order to show the difference between trilinear filtering/normal texture mapping more easily
            if(phong->trilinear) {
                // If we have mipmaps loaded, do trilinear filtering
                if(phong->diffuse_texture_tri_1 != nullptr && phong->diffuse_texture_tri_2 != nullptr &&
                        phong->diffuse_texture_tri_3 != nullptr) {
                    auto u = texcoord.x * 64;
                    auto v = texcoord.y * 64;
                    u -= floor(u);
                    v -= floor(v);
                    bool only_use_one = false;
                    Texture *tex_1, *tex_2;
                    float color_1_ratio, color_2_ratio;

                    // Perform trilinear blending based upon viewing distance
                    if(dist >= 0.0 && dist <= 4.0) {
                        tex_1 = phong->diffuse_texture;
                        tex_2 = phong->diffuse_texture_tri_1;

                        // Trilinear blending
                        color_1_ratio = 1 - dist / 4.0;
                        color_2_ratio = 1 - color_1_ratio;
                    }
                    else if(dist > 4.0 && dist <= 8.0) {
                        tex_1 = phong->diffuse_texture_tri_1;
                        tex_2 = phong->diffuse_texture_tri_2;

                        // Trilinear blending
                        color_1_ratio = 1 - (dist - 4.0) / 4.0;
                        color_2_ratio = 1 - color_1_ratio;
                    }
                    else if(dist > 8.0 && dist <= 16.0) {
                        tex_1 = phong->diffuse_texture_tri_2;
                        tex_2 = phong->diffuse_texture_tri_3;

                        // Trilinear blending
                        color_1_ratio = 1 - (dist - 8.0) / 8.0;
                        color_2_ratio = 1 - color_1_ratio;
                    }
                    else {
                        tex_1 = phong->diffuse_texture_tri_3;
                        only_use_one = true;
                    }

                    // Perform bilinear filtering
                    image3f &img_1 = tex_1->image;
                    float u_1 = u * img_1.width() - 0.5;
                    float v_1 = v * img_1.height() - 0.5;
                    float x_1 = floor(u_1);
                    float y_1 = floor(v_1);
                    if(x_1 < 0)
                        x_1 = 0.0;
                    if(y_1 < 0)
                        y_1 = 0.0;
                    if(x_1 > img_1.width() - 2)
                        x_1 = img_1.width() - 2;
                    if(y_1 > img_1.height() - 2)
                        y_1 = img_1.height() - 2;
                    float u_1_ratio = u_1 - x_1;
                    float v_1_ratio = v_1 - y_1;
                    float u_1_opposite = 1 - u_1_ratio;
                    float v_1_opposite = 1 - v_1_ratio;

                    auto color_1 = (img_1.at(x_1, y_1) * u_1_opposite + img_1.at(x_1 + 1, y_1) * u_1_ratio) *
                            v_1_opposite + (img_1.at(x_1, y_1 + 1) * u_1_opposite + img_1.at(x_1 + 1, y_1 + 1) * u_1_ratio) * v_1_ratio;

                    // If we're not too far away from the image, blend with another mipmap
                    if(!only_use_one) {
                        image3f &img_2 = tex_2->image;
                        float u_2 = u * img_2.width() - 0.5;
                        float v_2 = v * img_2.height() - 0.5;
                        float x_2 = floor(u_2);
                        float y_2 = floor(v_2);
                        if(x_2 < 0)
                            x_2 = 0.0;
                        if(y_2 < 0)
                            y_2 = 0.0;
                        if(x_2 > img_2.width() - 2)
                            x_2 = img_2.width() - 2;
                        if(y_2 > img_2.height() - 2)
                            y_2 = img_2.height() - 2;
                        float u_2_ratio = u_2 - x_2;
                        float v_2_ratio = v_2 - y_2;
                        float u_2_opposite = 1 - u_2_ratio;
                        float v_2_opposite = 1 - v_2_ratio;

                        auto color_2 = (img_2.at(x_2, y_2) * u_2_opposite + img_2.at(x_2 + 1, y_2) * u_2_ratio) *
                                v_2_opposite + (img_2.at(x_2, y_2 + 1) * u_2_opposite + img_2.at(x_2 + 1, y_2 + 1) * u_2_ratio) * v_2_ratio;

                        ret->diffuse += color_1 * color_1_ratio + color_2 * color_2_ratio;
                    }
                    else
                        ret->diffuse += color_1;
                }
                // If we don't have mipmaps loaded, perform a heavily patterned normal texture map
                else if(phong->diffuse_texture != nullptr) {
                    auto x = texcoord.x * 64;
                    x -= floor(x);
                    auto y = texcoord.y * 64;
                    y -= floor(y);

                    ret->diffuse += phong->diffuse_texture->image.at(x * phong->diffuse_texture->image.width(),
                                      y * phong->diffuse_texture->image.height());
                }
            }
            // Texture mapping (5 points)
            // Perform simple texture mapping
            else {
                if(phong->diffuse_texture != nullptr) {
                    ret->diffuse += phong->diffuse_texture->image.at(texcoord.x * phong->diffuse_texture->image.width(),
                                      texcoord.y * phong->diffuse_texture->image.height());
                }
            }
        }
        ret->specular = phong->specular;
        ret->exponent = phong->exponent;
        ret->reflection = phong->reflection;
        return ret;
    }
    else if(is<LambertEmission>(material)) {
        auto emission = cast<LambertEmission>(material);
        auto ret = new LambertEmission();
        ret->diffuse = emission->diffuse;
        ret->emission = emission->emission;
        return ret;
    }
    else { NOT_IMPLEMENTED_ERROR(); return nullptr; }
}

/// evaluate the material color
inline vec3f material_diffuse_albedo(Material* material) {
    ERROR_IF_NOT(not material_has_textures(material), "cannot support textures");
    if(is<Lambert>(material)) return cast<Lambert>(material)->diffuse;
    else if(is<Phong>(material)) return cast<Phong>(material)->diffuse;
    else if(is<LambertEmission>(material)) return cast<LambertEmission>(material)->diffuse;
    else { NOT_IMPLEMENTED_ERROR(); return zero3f; }
}

/// evaluete the emission of the material
inline vec3f material_emission(Material* material, const frame3f& frame, const vec3f& wo) {
    ERROR_IF_NOT(not material_has_textures(material), "cannot support textures");
    if(is<LambertEmission>(material)) {
        auto lambert = cast<LambertEmission>(material);
        if(dot(wo,frame.z) <= 0) return zero3f;
        return lambert->emission;
    } else return zero3f;
}

/// evaluate an approximation of the fresnel model
inline vec3f _schlickFresnel(const vec3f& rhos, float iDh) {
    return rhos + (vec3f(1,1,1)-rhos) * pow(1.0f-iDh,5.0f);
}

/// evaluate an approximation of the fresnel model
inline vec3f _schlickFresnel(const vec3f& rhos, const vec3f& w, const vec3f& wh) {
    return _schlickFresnel(rhos, dot(wh,w));
}

/// evaluate product of BRDF and cosine
inline vec3f material_brdfcos(Material* material, const frame3f& frame, const vec3f& wi, const vec3f& wo) {
    ERROR_IF_NOT(not material_has_textures(material), "cannot support textures");
    if(is<Lambert>(material)) {
        auto lambert = cast<Lambert>(material);
        if(dot(wi,frame.z) <= 0 or dot(wo,frame.z) <= 0) return zero3f;
        return lambert->diffuse * abs(dot(wi,frame.z)) / pif;
    }
    else if(is<Phong>(material)) {
        auto phong = cast<Phong>(material);
        if(dot(wi,frame.z) <= 0 or dot(wo,frame.z) <= 0) return zero3f;
        if(phong->use_reflected) {
            vec3f wr = reflect(-wi,frame.z);
            return (phong->diffuse / pif + (phong->exponent + 8) * phong->specular*pow(max(dot(wo,wr),0.0f),phong->exponent) / (8*pif)) * abs(dot(wi,frame.z));
        } else {
            vec3f wh = normalize(wi+wo);
            return (phong->diffuse / pif + (phong->exponent + 8) * phong->specular*pow(max(dot(frame.z,wh),0.0f),phong->exponent) / (8*pif)) * abs(dot(wi,frame.z));
        }
    }
    else if(is<LambertEmission>(material)) {
        auto lambert = cast<LambertEmission>(material);
        if(dot(wi,frame.z) <= 0 or dot(wo,frame.z) <= 0) return zero3f;
        return lambert->diffuse * abs(dot(wi,frame.z)) / pif;
    }
    else { NOT_IMPLEMENTED_ERROR(); return zero3f; }
}

/// material average color for interactive drawing
inline vec3f material_display_color(Material* material) {
    if(is<Lambert>(material)) return cast<Lambert>(material)->diffuse;
    else if(is<Phong>(material)) return cast<Phong>(material)->diffuse;
    else if(is<LambertEmission>(material)) return cast<LambertEmission>(material)->emission;
    else { NOT_IMPLEMENTED_ERROR(); return zero3f; }
}
///@}

///@name sample interface
///@{
struct BrdfSample {
    vec3f           brdfcos = zero3f;
    vec3f           wi = zero3f;
    float           pdf = 1;
};

/// evaluate color and direction of mirror reflection (zero if not reflections)
inline BrdfSample material_sample_reflection(Material* material, const frame3f& frame, const vec3f& wo) {
    ERROR_IF_NOT(not material_has_textures(material), "no textures allowed");
    if(is<Phong>(material)) {
        auto phong = cast<Phong>(material);
        if(dot(wo,frame.z) <= 0) return BrdfSample();
        auto bs = BrdfSample();
        bs.brdfcos = phong->reflection;
        bs.wi = reflect(-wo, frame.z);
        bs.pdf = 1;
        return bs;
    }
    else { return BrdfSample(); }
}

/// evaluate color and direction of blurred mirror reflection (zero if not reflections)
inline BrdfSample material_sample_blurryreflection(Material* material, const frame3f& frame, const vec3f& wo, const vec2f& suv) {
    ERROR_IF_NOT(not material_has_textures(material), "no textures allowed");
    auto bs = BrdfSample();
    if(is<Phong>(material)) {
        auto phong = cast<Phong>(material);
        if(dot(wo,frame.z) <= 0) return BrdfSample();
        auto wi = reflect(-wo, frame.z);
        auto u = normalize(cross(wi, wo));
        auto v = normalize(cross(wi, u));
        auto sl = phong->blur_size;
        
        bs.brdfcos = phong->reflection;
        bs.wi = normalize(wi + (0.5f-suv.x)*sl*u + (0.5f-suv.y)*sl*v);
        bs.pdf = 1.0/(sl*sl);
        
        return bs;
    }
    else { return bs; }
}

/// pick a direction and sample it
inline BrdfSample material_sample_brdfcos(Material* material, const frame3f& frame, const vec3f& wo, const vec2f& suv, float sl) {
    ERROR_IF_NOT(not material_has_textures(material), "no textures allowed");
    if(is<Lambert>(material)) {
        auto lambert = cast<Lambert>(material);
        if(dot(wo,frame.z) <= 0) return BrdfSample();
        auto ds = sample_direction_hemisphericalcos(suv);
        auto wi = transform_direction(frame, ds.dir);
        BrdfSample bs;
        bs.brdfcos = material_brdfcos(lambert, frame, wi, wo);
        bs.wi = wi;
        bs.pdf = ds.pdf;
        return bs;
    }
    else if(is<Phong>(material)) {
        auto phong = cast<Phong>(material);
        if(dot(wo,frame.z) <= 0) return BrdfSample();
        auto ds = sample_direction_hemisphericalcos(suv);
        auto wi = transform_direction(frame, ds.dir);
        BrdfSample bs;
        bs.brdfcos = material_brdfcos(phong, frame, wi, wo);
        bs.wi = wi;
        bs.pdf = ds.pdf;
        return bs;
    }
    else if(is<LambertEmission>(material)) {
        auto lambert = cast<LambertEmission>(material);
        if(dot(wo,frame.z) <= 0) return BrdfSample();
        auto ds = sample_direction_hemisphericalcos(suv);
        auto wi = transform_direction(frame, ds.dir);
        BrdfSample bs;
        bs.brdfcos = material_brdfcos(lambert, frame, wi, wo);
        bs.wi = wi;
        bs.pdf = ds.pdf;
        return bs;
    }
    else { NOT_IMPLEMENTED_ERROR(); return BrdfSample(); }
}

///@}

///@}

#endif
