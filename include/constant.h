#ifndef CONSTANT_H__
#define CONSTANT_H__

namespace constant {
  namespace math {
    constexpr float pi = 3.14159265358979323846f;
  }

  namespace physics {
    constexpr float G = 6.67430e-11f;
    constexpr float c = 299792458.f;
    constexpr float sigma = 5.670367e-8f;

    constexpr float M_Sun = 1.989e30f /* kg */;
    constexpr float ly = 9454254955488000.f /* m */;
  }

  namespace model {
    constexpr float M = 1.49e7f /* M_SUN */;
    constexpr float M_kg = M * physics::M_Sun;
    constexpr float Rs_m = 2. * physics::G * M * physics::M_Sun / physics::c / physics::c /* m */;
    constexpr float Rs = Rs_m / physics::ly /* ly */;
    
    constexpr float disk_thickness = 0.8 /* Rs */;
    constexpr float Rin = 2.5 /* Rs */;
    constexpr float Rout = 8 /* Rs */;

    constexpr float disk_luminous_intensity = 1.2;
    constexpr float disk_opacity = 0.65;

    constexpr float shape_noise_detail_coef = 3;
    constexpr float shape_noise_superposition_intensity = 0.1;
    constexpr float shape_noise_contrast = 80;

    constexpr int shape_noise_detail_level = 3;
    constexpr int cloud_noise_detail_level = 7;
    constexpr float noise_frequency0 = 1.15;

    constexpr float temperature_coeff = 1.4;
  }

  namespace renderer {
    constexpr float outer_range = 100.;
    constexpr int step_limit = 20000;
    constexpr float dl0 = 0.15;
  }

  namespace camera0 {
    constexpr float focal_length = 8.5e-2;
    constexpr float width = 9.6e-2;
    constexpr float camera_r = 11;      /* distance between camera and origin */
    constexpr float phi = 0.02;         /* angle of camera and x-axis */
    constexpr float theta = 0;          /* angle of rotation of view direction along up-world */
    constexpr float elevation = 0;      /* angle of elevation */
    constexpr float alpha = 0.2;        /* angle of rotation along view direction */
  }

  namespace camera1 {
    constexpr float focal_length = 1.1e-1;
    constexpr float width = 9.6e-2;
    constexpr float camera_r = 7;       /* distance between camera and origin */
    constexpr float phi = 0.1;          /* angle of camera and x-axis */
    constexpr float theta = 0.4;        /* angle of rotation of view direction along up-world */
    constexpr float elevation = 0.08;   /* angle of elevation */
    constexpr float alpha = 0.1;        /* angle of rotation along view direction */
  }

  namespace image {
    constexpr int height_px = 200;
    constexpr int width_px = 300;
  }
}


#endif