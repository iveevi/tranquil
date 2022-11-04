use std::ops;

// Math functions
fn radians(degrees: f32) -> f32 {
    degrees * std::f32::consts::PI / 180.0
}

// Vector types and functions
#[derive(Copy, Clone, Debug)]
pub struct Vec3 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

pub fn length(v: Vec3) -> f32 {
    (v.x * v.x + v.y * v.y + v.z * v.z).sqrt()
}

pub fn dot(v1: Vec3, v2: Vec3) -> f32 {
    v1.x * v2.x + v1.y * v2.y + v1.z * v2.z
}

pub fn normalize(v: Vec3) -> Vec3 {
    let l = length(v);
    Vec3 {
        x: v.x / l,
        y: v.y / l,
        z: v.z / l
    }
}

pub fn cross(a: Vec3, b: Vec3) -> Vec3 {
    Vec3 {
        x: a.y * b.z - a.z * b.y,
        y: a.z * b.x - a.x * b.z,
        z: a.x * b.y - a.y * b.x,
    }
}

// Matrix type
#[derive(Copy, Clone, Debug)]
pub struct Matrix {
    pub data: [[f32; 4]; 4],
}

// Matrix functions
impl Matrix {
    // Get position and axes
    pub fn position(&self) -> Vec3 {
        Vec3 {
            x: self.data[0][3],
            y: self.data[1][3],
            z: self.data[2][3],
        }
    }

    pub fn forward(&self) -> Vec3 {
        Vec3 {
            x: self.data[0][2],
            y: self.data[1][2],
            z: self.data[2][2],
        }
    }

    pub fn up(&self) -> Vec3 {
        Vec3 {
            x: self.data[0][1],
            y: self.data[1][1],
            z: self.data[2][1],
        }
    }

    pub fn right(&self) -> Vec3 {
        Vec3 {
            x: self.data[0][0],
            y: self.data[1][0],
            z: self.data[2][0],
        }
    }

    // Matrix generators
    pub fn identity() -> Matrix {
        Matrix {
            data: [
                [1.0, 0.0, 0.0, 0.0],
                [0.0, 1.0, 0.0, 0.0],
                [0.0, 0.0, 1.0, 0.0],
                [0.0, 0.0, 0.0, 1.0f32]
            ]
        }
    }

    // Assume fov to be in degrees
    pub fn perspective(fov: f32, aspect: f32, znear: f32, zfar: f32) -> Matrix {
        assert!(fov > 0.0 && fov < 180.0);
        assert!(znear > 0.0);
        assert!(zfar > znear);

        let rfov = radians(fov);
        let f = 1.0/(rfov/2.0).tan();

        Matrix {
            data: [
                [aspect/f, 0.0, 0.0, 0.0],
                [0.0, f, 0.0, 0.0],
                [0.0, 0.0, (zfar + znear)/(zfar - znear), 1.0],
                [0.0, 0.0, -(2.0 * zfar * znear)/(zfar - znear), 0.0]
            ]
        }
    }

    // Look at matrix
    pub fn look_at(eye: Vec3, center: Vec3, up: Vec3) -> Matrix {
        let f = normalize(center - eye);
        let u = normalize(up);
        let s = normalize(cross(f, u));

        Matrix {
            data: [
                [s.x, u.x, -f.x, 0.0],
                [s.y, u.y, -f.y, 0.0],
                [s.z, u.z, -f.z, 0.0],
                [-dot(s, eye), -dot(u, eye), dot(f, eye), 1.0],
            ]
        }
    }
}

// Arithmetic
impl ops::Add for Vec3 {
    type Output = Vec3;

    fn add(self, other: Vec3) -> Vec3 {
        Vec3 {
            x: self.x + other.x,
            y: self.y + other.y,
            z: self.z + other.z,
        }
    }
}

impl ops::Sub for Vec3 {
    type Output = Vec3;

    fn sub(self, other: Vec3) -> Vec3 {
        Vec3 {
            x: self.x - other.x,
            y: self.y - other.y,
            z: self.z - other.z,
        }
    }
}
