import os
import math
from PIL import Image

def distance_point_to_segment(px, py, x1, y1, x2, y2):
    dx = x2 - x1
    dy = y2 - y1
    l2 = dx * dx + dy * dy
    if l2 == 0.0:
        return math.hypot(px - x1, py - y1)
    t = max(0.0, min(1.0, ((px - x1) * dx + (py - y1) * dy) / l2))
    proj_x = x1 + t * dx
    proj_y = y1 + t * dy
    return math.hypot(px - proj_x, py - proj_y)

def create_coal_textures():
    size = 64
    cx, cy = 32.0, 32.0

    # Angular polygon approximating a raw chunk of anthracite coal (12 vertices)
    poly_angles_deg = [0, 32, 65, 95, 128, 162, 195, 222, 250, 285, 315, 342]
    poly_radii = [23.0, 24.5, 22.0, 25.0, 23.5, 24.0, 25.5, 23.0, 24.5, 22.5, 24.0, 23.0]

    # Convert to polygon points
    poly = []
    for deg, r in zip(poly_angles_deg, poly_radii):
        rad = math.radians(deg)
        poly.append((cx + r * math.cos(rad), cy + r * math.sin(rad)))

    # Point in polygon test (standard ray casting)
    def is_inside_poly(x, y):
        inside = False
        n = len(poly)
        for i in range(n):
            p1x, p1y = poly[i]
            p2x, p2y = poly[(i + 1) % n]
            if ((p1y > y) != (p2y > y)) and (x < (p2x - p1x) * (y - p1y) / (p2y - p1y) + p1x):
                inside = not inside
        return inside

    # Minimum distance to polygon boundary (positive inside, negative outside)
    def dist_to_boundary(x, y):
        min_dist = 1e9
        n = len(poly)
        for i in range(n):
            p1x, p1y = poly[i]
            p2x, p2y = poly[(i + 1) % n]
            d = distance_point_to_segment(x, y, p1x, p1y, p2x, p2y)
            if d < min_dist:
                min_dist = d
        inside = is_inside_poly(x, y)
        return min_dist if inside else -min_dist

    # Thin, asymmetric cracks hugging outer stone facets / chips,
    # strictly keeping the central face area (x in [22, 42], y in [22, 40]) pristine and dark.
    cracks = [
        # Top-Left shoulder / facet
        ((24.0, 13.0), (19.0, 17.0)),
        ((19.0, 17.0), (15.0, 22.0)),
        ((19.0, 17.0), (21.0, 20.0)),
        # Top-Right shoulder / facet
        ((39.0, 13.0), (44.0, 16.0)),
        ((44.0, 16.0), (49.0, 21.0)),
        ((44.0, 16.0), (41.0, 19.0)),
        # Right flank
        ((51.0, 29.0), (47.0, 33.0)),
        ((47.0, 33.0), (49.0, 39.0)),
        ((47.0, 33.0), (45.0, 34.0)),
        # Bottom-Right corner
        ((46.0, 44.0), (41.0, 47.0)),
        ((41.0, 47.0), (36.0, 51.5)),
        ((41.0, 47.0), (43.0, 48.5)),
        # Bottom-Left corner
        ((14.0, 34.0), (18.0, 40.0)),
        ((18.0, 40.0), (22.0, 47.0)),
        ((18.0, 40.0), (16.0, 44.5)),
        # Lower chin notch
        ((27.0, 52.0), (30.0, 48.0)),
        ((30.0, 48.0), (33.0, 50.0)),
    ]

    # Expressive combat elemental eyes (angled / frowning towards bridge of the nose)
    def eye_dist(px, py, is_right):
        ec_x = 39.5 if is_right else 24.5
        ec_y = 26.0
        angle = -20.0 if is_right else 20.0
        rad = math.radians(angle)
        dx = px - ec_x
        dy = py - ec_y
        rx = dx * math.cos(rad) + dy * math.sin(rad)
        ry = -dx * math.sin(rad) + dy * math.cos(rad)
        if not is_right:
            rx = -rx
        width = 3.8
        height = 1.7 if ry < 0 else 2.1
        taper = 1.0 - 0.12 * rx
        return math.hypot(rx / width, (ry * taper) / height)

    # Neat zigzag toothy glowing mouth with a slight smirk below the eyes
    m_pts = [
        (26.5, 36.8),
        (28.5, 35.0),
        (30.5, 37.0),
        (32.0, 34.6),
        (33.5, 36.8),
        (35.5, 34.8),
        (37.5, 35.8)
    ]

    # Directional lighting from top-left for 3D anthracite stone facets
    lx, ly, lz = -0.5, -0.6, 0.62
    llen = math.hypot(lx, ly, lz)
    lx, ly, lz = lx / llen, ly / llen, lz / llen

    img_coal = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    img_glow = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    pix_coal = img_coal.load()
    pix_glow = img_glow.load()

    for y in range(size):
        for x in range(size):
            px = x + 0.5
            py = y + 0.5

            d_bound = dist_to_boundary(px, py)

            # Strict rule 1: Outside polygon silhouette MUST be completely transparent (0, 0, 0, 0)
            if d_bound <= 0.0:
                continue

            outer_alpha = min(1.0, d_bound)

            # Faceted normal for anthracite rock chunk
            dx = px - cx
            dy = py - cy
            dist_c = math.hypot(dx, dy)

            facet_u = math.sin(px * 0.45 + py * 0.15) * 0.35 + math.cos(px * 0.2 - py * 0.4) * 0.35
            facet_v = math.cos(px * 0.35 - py * 0.25) * 0.35 + math.sin(px * 0.15 + py * 0.35) * 0.35

            nx = (dx / 25.0) * 0.7 + facet_u * 0.3
            ny = (dy / 25.0) * 0.7 + facet_v * 0.3
            n_len = math.hypot(nx, ny)
            if n_len > 0.95:
                nx = (nx / n_len) * 0.95
                ny = (ny / n_len) * 0.95
            nz = math.sqrt(max(0.0, 1.0 - (nx * nx + ny * ny)))

            dot_nl = max(0.0, nx * lx + ny * ly + nz * lz)
            spec = math.pow(max(0.0, 2.0 * dot_nl * nz - lz), 12.0) * 0.45

            # Base anthracite color: deep charcoal graphite
            base_r = 28.0 + dot_nl * 26.0 + spec * 75.0
            base_g = 30.0 + dot_nl * 28.0 + spec * 80.0
            base_b = 35.0 + dot_nl * 32.0 + spec * 90.0

            rim_factor = min(1.0, max(0.2, d_bound / 3.0))
            base_r *= (0.45 + 0.55 * rim_factor)
            base_g *= (0.45 + 0.55 * rim_factor)
            base_b *= (0.45 + 0.55 * rim_factor)

            # Distances to features
            min_crack_dist = min(distance_point_to_segment(px, py, c1[0], c1[1], c2[0], c2[1]) for c1, c2 in cracks)
            min_mouth_dist = min(distance_point_to_segment(px, py, p1[0], p1[1], p2[0], p2[1]) for p1, p2 in zip(m_pts[:-1], m_pts[1:]))
            de_l = eye_dist(px, py, False)
            de_r = eye_dist(px, py, True)
            min_eye_dist = min(de_l, de_r)

            cr, cg, cb = base_r, base_g, base_b

            # Charring around fissures, mouth, and eyes
            if min_crack_dist < 1.8:
                char = max(0.0, 1.0 - min_crack_dist / 1.8) * 0.55
                cr *= (1.0 - char)
                cg *= (1.0 - char)
                cb *= (1.0 - char)

            if min_mouth_dist < 2.2:
                char = max(0.0, 1.0 - min_mouth_dist / 2.2) * 0.65
                cr *= (1.0 - char)
                cg *= (1.0 - char)
                cb *= (1.0 - char)

            if min_eye_dist < 1.5:
                char = max(0.0, 1.0 - min_eye_dist / 1.5) * 0.5
                cr *= (1.0 - char)
                cg *= (1.0 - char)
                cb *= (1.0 - char)

            # Thin glowing crack embers on base sprite
            crack_glow = 0.0
            if min_crack_dist < 2.2 and d_bound > 1.2:
                crack_glow = math.exp(-min_crack_dist * 1.6) * 0.8
                if min_crack_dist < 0.9:
                    t = max(0.0, 1.0 - min_crack_dist / 0.9)
                    cr = cr * (1.0 - t) + 255.0 * t
                    cg = cg * (1.0 - t) + (100.0 + 130.0 * t) * t
                    cb = cb * (1.0 - t) + (15.0 + 55.0 * t) * t

            # Jagged glowing mouth on base sprite
            mouth_glow = 0.0
            if min_mouth_dist < 2.5 and d_bound > 1.2:
                mouth_glow = math.exp(-min_mouth_dist * 1.4) * 0.95
                if min_mouth_dist < 1.0:
                    t = max(0.0, 1.0 - min_mouth_dist / 1.0)
                    cr = cr * (1.0 - t) + 255.0 * t
                    cg = cg * (1.0 - t) + (110.0 + 130.0 * t) * t
                    cb = cb * (1.0 - t) + (20.0 + 60.0 * t) * t

            # Fiery combat eyes: bright yellow center with rich orange surround
            eye_glow = 0.0
            if min_eye_dist < 1.7 and d_bound > 1.2:
                eye_glow = math.exp(-min_eye_dist * 1.5) * 1.4
                if min_eye_dist < 0.55:
                    t = min_eye_dist / 0.55
                    eye_r = 255.0
                    eye_g = 245.0 - 55.0 * t
                    eye_b = 90.0 - 70.0 * t
                    cr, cg, cb = eye_r, eye_g, eye_b
                elif min_eye_dist < 0.95:
                    t = (min_eye_dist - 0.55) / 0.40
                    eye_r = 255.0
                    eye_g = 190.0 - 100.0 * t
                    eye_b = 20.0 - 10.0 * t
                    cr, cg, cb = eye_r, eye_g, eye_b
                elif min_eye_dist < 1.35:
                    t = (min_eye_dist - 0.95) / 0.40
                    eye_r = 255.0 - 80.0 * t
                    eye_g = 90.0 - 55.0 * t
                    eye_b = 10.0
                    blend = 1.0 - t
                    cr = cr * (1.0 - blend) + eye_r * blend
                    cg = cg * (1.0 - blend) + eye_g * blend
                    cb = cb * (1.0 - blend) + eye_b * blend

            final_a = int(round(outer_alpha * 255))
            if final_a > 0:
                pix_coal[x, y] = (
                    int(round(max(0, min(255, cr)))),
                    int(round(max(0, min(255, cg)))),
                    int(round(max(0, min(255, cb)))),
                    final_a
                )

            # Pure Emissive Glow Layer (coal_glow.png)
            # Strictly contained within rock silhouette:
            # Fades to 0 when d_bound < 1.2 to guarantee zero aura/leak outside rock
            total_glow = max(crack_glow, mouth_glow, eye_glow)
            glow_bound_mask = max(0.0, min(1.0, (d_bound - 1.2) / 1.0))
            effective_glow = total_glow * glow_bound_mask

            if effective_glow > 0.03:
                g_intensity = min(1.0, effective_glow)
                glow_r = 255.0
                glow_g = min(255.0, 50.0 + 205.0 * math.pow(g_intensity, 1.2))
                glow_b = min(255.0, 10.0 + 225.0 * math.pow(g_intensity, 2.2))
                glow_alpha = min(1.0, g_intensity * 1.2) * outer_alpha

                pix_glow[x, y] = (
                    int(round(max(0, min(255, glow_r)))),
                    int(round(max(0, min(255, glow_g)))),
                    int(round(max(0, min(255, glow_b)))),
                    int(round(max(0, min(255, glow_alpha * 255))))
                )

    return img_coal, img_glow

if __name__ == '__main__':
    img_coal, img_glow = create_coal_textures()

    output_dirs = [
        'res/textures/enemies',
        'res/textures',
        'build/res/textures/enemies',
        'out/build/x64-Debug/res/textures/enemies'
    ]

    for d in output_dirs:
        if os.path.exists(d):
            coal_path = os.path.join(d, 'coal.png')
            glow_path = os.path.join(d, 'coal_glow.png')
            img_coal.save(coal_path)
            img_glow.save(glow_path)
            print(f"Saved textures to: {coal_path} and {glow_path}")

