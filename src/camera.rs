use std::f32::consts::FRAC_PI_2;

use cgmath::InnerSpace;
use cgmath::Matrix4;
use cgmath::PerspectiveFov;
use cgmath::Point3;
use cgmath::Rad;
use cgmath::Vector3;

// Camera
#[derive(Copy, Clone, Debug)]
pub struct Camera {
    pub fov: f32,
    pub position: Point3 <f32>,
    pub yaw: Rad <f32>,
    pub pitch: Rad <f32>,
}

impl Camera {
    // Constructor
    pub fn new() -> Camera {
        Camera {
            fov: 90.0,
            position: Point3::new(0.0, 0.0, 0.0),
            yaw: Rad(FRAC_PI_2),
            pitch: Rad(0.0),
        }
    }

    // Matrices
    pub fn view(&self) -> Matrix4 <f32> {
        Matrix4::look_to_rh(
            self.position,
            Vector3::new(
                self.yaw.0.cos(),
                self.pitch.0.sin(),
                self.yaw.0.sin(),
            ).normalize(),
            Vector3::unit_y(),
        )
    }

    pub fn perspective(&self) -> Matrix4 <f32> {
        PerspectiveFov {
            fovy: Rad(self.fov.to_radians()),
            aspect: 1.0,
            near: 0.1,
            far: 100.0,
        }.into()
    }

    // Axes
    pub fn forward(&self) -> Vector3 <f32> {
        Vector3::new(
            self.yaw.0.cos(), 0.0,
            self.yaw.0.sin(),
        ).normalize()
    }

    pub fn right(&self) -> Vector3 <f32> {
        Vector3::new(
            -self.yaw.0.sin(), 0.0,
            self.yaw.0.cos(),
        ).normalize()
    }
}
