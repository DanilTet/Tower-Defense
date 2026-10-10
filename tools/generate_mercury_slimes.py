import os
import math
from PIL import Image

def generate_mercury_slime(radius: float, img_size: int) -> Image.Image:
    """
    Generates a metallic chrome liquid mercury slime sprite with anti-aliasing,
    volumetric hemisphere normals, liquid chrome reflection bands, and specular highlights.
    """
    img = Image.new('RGBA', (img_size, img_size), (0, 0, 0, 0))
    pixels = img.load()
    cx = img_size / 2.0
    cy = img_size / 2.0

    # Key light direction (normalized) from top-left
    lx, ly, lz = -0.45, -0.55, 0.70
    l_len = math.sqrt(lx * lx + ly * ly + lz * lz)
    lx, ly, lz = lx / l_len, ly / l_len, lz / l_len

    for y in range(img_size):
        for x in range(img_size):
            dx = (x + 0.5) - cx
            dy = (y + 0.5) - cy
            dist = math.sqrt(dx * dx + dy * dy)

            if dist > radius + 1.5:
                continue

            # Anti-aliasing alpha at outer border
            edge = radius - dist
            alpha = max(0.0, min(1.0, edge + 0.5))

            # Surface normal on hemisphere
            rn = min(dist / radius, 0.999)
            nx = dx / radius
            ny = dy / radius
            nz = math.sqrt(max(0.0, 1.0 - rn * rn))

            # Lambertian diffuse
            dot_nl = max(0.0, nx * lx + ny * ly + nz * lz)

            # Specular reflections
            rx = 2.0 * dot_nl * nx - lx
            ry = 2.0 * dot_nl * ny - ly
            rz = 2.0 * dot_nl * nz - lz
            dot_rv = max(0.0, rz)
            spec1 = math.pow(dot_rv, 18.0) * 1.3
            spec2 = math.pow(dot_rv, 6.0) * 0.4

            # Fresnel rim light
            fresnel = math.pow(1.0 - nz, 2.5)

            # Chrome metallic horizon reflections
            env_ref = 0.5 + 0.5 * math.sin(ny * 3.5 + nz * 2.0)

            # Metallic chrome base shading
            base_r = 110.0 + 90.0 * dot_nl + 40.0 * env_ref + fresnel * 90.0
            base_g = 120.0 + 95.0 * dot_nl + 45.0 * env_ref + fresnel * 95.0
            base_b = 135.0 + 105.0 * dot_nl + 55.0 * env_ref + fresnel * 105.0

            # Subtle ground bounce light
            bounce = max(0.0, 0.4 * nx + 0.5 * ny) * (1.0 - nz) * 50.0
            base_r += bounce * 0.8
            base_g += bounce * 0.9
            base_b += bounce * 1.1

            final_r = min(255, int(base_r + (spec1 + spec2) * 230.0))
            final_g = min(255, int(base_g + (spec1 + spec2) * 235.0))
            final_b = min(255, int(base_b + (spec1 + spec2) * 250.0))
            final_a = int(alpha * 255.0)

            pixels[x, y] = (final_r, final_g, final_b, final_a)

    return img

if __name__ == '__main__':
    os.makedirs('res/textures/enemies', exist_ok=True)
    img_large = generate_mercury_slime(40.0, 96)
    img_large.save('res/textures/enemies/mercury_slime_large.png')
    img_large.save('res/textures/mercury_slime_large.png')

    img_medium = generate_mercury_slime(26.0, 64)
    img_medium.save('res/textures/enemies/mercury_slime_medium.png')
    img_medium.save('res/textures/mercury_slime_medium.png')

    img_small = generate_mercury_slime(16.0, 40)
    img_small.save('res/textures/enemies/mercury_slime_small.png')
    img_small.save('res/textures/mercury_slime_small.png')
    print('Generated mercury slime textures successfully.')

