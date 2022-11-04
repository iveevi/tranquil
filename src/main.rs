use std::fs;
use std::ops;

#[macro_use]
extern crate glium;

// Vector types
#[derive(Copy, Clone)]
struct Vec3 {
    x: f32,
    y: f32,
    z: f32,
}

#[derive(Copy, Clone)]
struct Vertex {
    position: [f32; 3],
}

// Math functions
fn radians(degrees: f32) -> f32 {
    degrees * std::f32::consts::PI / 180.0
}

fn length(v: Vec3) -> f32 {
    (v.x * v.x + v.y * v.y + v.z * v.z).sqrt()
}

fn dot(v1: Vec3, v2: Vec3) -> f32 {
    v1.x * v2.x + v1.y * v2.y + v1.z * v2.z
}

fn normalize(v: Vec3) -> Vec3 {
    let l = length(v);
    Vec3 {
        x: v.x / l,
        y: v.y / l,
        z: v.z / l
    }
}

fn cross(a: Vec3, b: Vec3) -> Vec3 {
    Vec3 {
        x: a.y * b.z - a.z * b.y,
        y: a.z * b.x - a.x * b.z,
        z: a.x * b.y - a.y * b.x,
    }
}

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

implement_vertex!(Vertex, position);

// Matrix type
#[derive(Copy, Clone, Debug)]
struct Matrix {
    data: [[f32; 4]; 4],
}

// Matrix functions
impl Matrix {
    // Get position and axes
    fn position(&self) -> Vec3 {
        Vec3 {
            x: self.data[0][3],
            y: self.data[1][3],
            z: self.data[2][3],
        }
    }

    fn forward(&self) -> Vec3 {
        Vec3 {
            x: self.data[0][2],
            y: self.data[1][2],
            z: self.data[2][2],
        }
    }

    fn up(&self) -> Vec3 {
        Vec3 {
            x: self.data[0][1],
            y: self.data[1][1],
            z: self.data[2][1],
        }
    }

    fn right(&self) -> Vec3 {
        Vec3 {
            x: self.data[0][0],
            y: self.data[1][0],
            z: self.data[2][0],
        }
    }

    // Matrix generators
    fn identity() -> Matrix {
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
    fn perspective(fov: f32, aspect: f32, znear: f32, zfar: f32) -> Matrix {
        assert!(fov > 0.0 && fov < 180.0);
        assert!(znear > 0.0);
        assert!(zfar > znear);

        let rfov = radians(fov);
        let hfov = (rfov/2.0).tan();

        Matrix {
            data: [
                [1.0/(aspect * hfov), 0.0, 0.0, 0.0],
                [0.0, 1.0/hfov, 0.0, 0.0],
                [0.0, 0.0, -(zfar + znear)/(zfar - znear), -1.0],
                [0.0, 0.0, -(2.0 * zfar * znear)/(zfar - znear), 0.0]
            ]
        }
    }

    // Look at matrix
    fn look_at(eye: Vec3, center: Vec3, up: Vec3) -> Matrix {
        let f = normalize(center - eye);
        let u = normalize(up);
        let s = normalize(cross(f, u));

        Matrix {
            data: [
                [s.x, s.y, s.z, -dot(s, eye)],
                [u.x, u.y, u.z, -dot(u, eye)],
                [-f.x, -f.y, -f.z, dot(f, eye)],
                [0.0, 0.0, 0.0, 1.0]
            ]
        }
    }
}

// Camera
#[derive(Copy, Clone, Debug)]
struct Camera {
    fov: f32,
    transform: Matrix,
}

impl Camera {
    fn new() -> Camera {
        Camera {
            fov: 90.0,
            transform: Matrix::identity(),
        }
    }

    fn view(&self) -> Matrix {
        Matrix::look_at(
            self.transform.position(),
            self.transform.position() + self.transform.forward(),
            self.transform.up()
        )
    }

    fn perspective(&self) -> Matrix {
        Matrix::perspective(self.fov, 1.0, 0.1, 100.0)
    }
}

fn main()
{
    let camera = Camera::new();
    println!("Camera: {:?}", camera);
    println!("View: {:?}", camera.view());

    // Read vertex and fragment shaders
    let vertex_shader_src = fs::read_to_string("src/base.vert")
        .expect("Failed to read vertex shader");

    let fragment_shader_src = fs::read_to_string("src/base.frag")
        .expect("Failed to read fragment shader");
}
