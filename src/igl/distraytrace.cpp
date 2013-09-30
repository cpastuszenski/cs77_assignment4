#include "distraytrace.h"

#include "vmath/random.h"
#include "intersect.h"

///@file igl/distraytrace.cpp Distribution Raytracing. @ingroup igl

vec3f _distraytrace_scene_ray(Scene* scene, const ray3f& ray, DistributionRaytraceOptions& opts, int depth) {
    // intersect
    intersection3f intersection;
    if(not intersect_scene_first(scene,ray,intersection)) return opts.background;

    // set up variables
    auto frame = intersection.frame;
    auto texcoord = intersection.texcoord;
    auto wo = -ray.d;
    auto material = intersection.material;

    // shading frame
    if(opts.doublesided) frame = faceforward(frame,ray.d);
    frame = material_shading_frame(material, frame, texcoord);

    // brdf
    auto brdf = material_shading_textures(intersection.material, intersection.texcoord, length(frame.o - scene->camera->frame.o));

    // compute ambient
    vec3f c = zero3f;

    // Ambient occlusion (5.0 points)
    // Perform ambient occlusion calculation
    if(opts.samples_ambient > 0) {
        int total_escaped_ss_rays = 0;
        for(int i = 0; i < opts.samples_ambient; i++) {
            float x = opts.rng.next_float();
            float y = opts.rng.next_float();
            auto ds = sample_direction_hemisphericalcos(vec2f(x,y));
            auto wi = transform_direction(frame, ds.dir);
            ray3f ray = ray3f(frame.o, wi);
            if(not intersect_scene_any(scene, ray)) total_escaped_ss_rays++;
        }
        float escaped_ratio = (float) total_escaped_ss_rays/ (float) opts.samples_ambient;
        c += opts.ambient * escaped_ratio * material_diffuse_albedo(brdf);
    }
    else
        c += opts.ambient * material_diffuse_albedo(brdf);

    // compute emission
    c += material_emission(brdf, frame, wo);

    // compute direct
    auto& ll = (opts.cameralights) ? scene->_cameralights : scene->lights;
    for(auto l : ll->lights) {
        ShadowSample ss;
        vec3f cl;
        vec3f acc = zero3f;
        // Distribution raytracing - soft shadows (5.0 points)
        // Perform distribution raytracing with soft shadows, as described in the lecture notes
        if(is<AreaLight>(l)) {
            auto area_light = cast<AreaLight>(l);
            for(int i = 0; i < area_light->shadow_samples; i++) {
                auto sample = vec2f(opts.rng.next_float(), opts.rng.next_float());
                ss = light_shadow_sample(l, frame.o, sample, true);
                auto wi = ss.dir;
                if(ss.radiance == zero3f) continue;
                cl = ss.radiance * material_brdfcos(brdf,frame,wi,wo) / ss.pdf;
                if(cl == zero3f) continue;
                if(opts.shadows) {
                    if(not intersect_scene_any(scene,ray3f::segment(frame.o,frame.o+ss.dir*ss.dist))) acc += cl;
                } else acc += cl;
            }
            c += acc / area_light->shadow_samples;
        }
        else {
            ss = light_shadow_sample(l, frame.o, zero2f, false);
            auto wi = ss.dir;
            if(ss.radiance == zero3f) continue;
            cl = ss.radiance * material_brdfcos(brdf,frame,wi,wo) / ss.pdf;
            if(cl == zero3f) continue;
            if(opts.shadows) {
                if(not intersect_scene_any(scene,ray3f::segment(frame.o,frame.o+ss.dir*ss.dist))) c += cl;
            } else c += cl;
        }
    }

    // recursively compute reflections
    if(opts.reflections and depth < opts.max_depth) {
        auto bs = material_sample_reflection(brdf, frame, wo);
        if(not (bs.brdfcos == zero3f)) {
            auto refl_ray = ray3f(frame.o,bs.wi);
            c += _distraytrace_scene_ray(scene, refl_ray, opts, depth+1) * bs.brdfcos;
        }
    }

    // cleanup
    delete brdf;

    // done
    return c;
}

void distraytrace_scene_progressive(ImageBuffer& buffer, Scene* scene, DistributionRaytraceOptions& opts) {
    auto w = buffer.width();
    auto h = buffer.height();

    int s2 = max(1,(int)sqrt(opts.samples));
    for(int j = 0; j < h; j ++) {
        for(int i = 0; i < w; i ++) {
            // Depth of field (2.5 points)
            // If enabled, perform distribution raytracing with DOF
            if(opts.DOF) {
                for(int k = 0; k < s2; k++) {
                    auto f = scene->camera->frame;
                    auto n = scene->camera->focus_dist;
                    float scale = (float) n / (float) scene->camera->image_dist;
                    float la = scene->camera->focus_aperture;
                    vec2f lp = vec2f(scene->camera->image_width/w, scene->camera->image_height/h);

                    vec2f ri, si;
                    // Disk sampling (2.5 points)
                    // If enabled, perform disk sampling
                    if(opts.disk) {
                        do{ri = opts.rng.next_vec2f();} while(length(2 * ri - one2f) > 1);
                        do{si = opts.rng.next_vec2f();} while(length(2 * si - one2f) > 1);
                    }
                    else {
                        ri = opts.rng.next_vec2f();
                        si = opts.rng.next_vec2f();
                    }

                    auto Fi = scene->camera->frame.o + (0.5f - si.x) * la * f.x + (0.5f - si.y) * la * f.y;
                    auto Qi = scene->camera->frame.o + ((i - w/2) * scale + 0.5f - ri.x) * lp.x * f.x + ((j - h/2) * scale + 0.5f - ri.y) * lp.y * f.y - n * f.z;

                    ray3f ray = ray3f(Fi, normalize(Qi - Fi));
                    buffer.accum.at(i,h-1-j) += _distraytrace_scene_ray(scene,ray,opts,0);
                    buffer.samples.at(i,h-1-j) += 1;
                }
            }
            else {
                auto cs = buffer.samples.at(i,h-1-j);
                auto ii = cs % s2; auto jj = cs / s2;
                float u = (i+(ii+0.5)/s2)/w;
                float v = (j+(jj+0.5)/s2)/h;
                ray3f ray = camera_ray(scene->camera,vec2f(u,v));
                buffer.accum.at(i,h-1-j) += _distraytrace_scene_ray(scene,ray,opts,0);
                buffer.samples.at(i,h-1-j) += 1;
            }
        }
    }
}

