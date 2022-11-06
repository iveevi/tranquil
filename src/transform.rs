use cgmath::Deg;
use cgmath::Euler;
use cgmath::Matrix4;
use cgmath::Quaternion;
use cgmath::Vector3;
use cgmath::Vector4;

#[derive(Copy, Clone)]
pub struct Transform {
    pub position: Vector3 <f32>,
    pub rotation: Vector3 <f32>, // In degrees
    pub scale: Vector3 <f32>,
}

impl Transform {
    pub fn new() -> Transform {
        Transform {
            position: Vector3::new(0.0, 0.0, 0.0),
            rotation: Vector3::new(0.0, 0.0, 0.0),
            scale: Vector3::new(1.0, 1.0, 1.0),
        }
    }

    pub fn matrix(&self) -> Matrix4 <f32> {
        let translate = Matrix4::from_translation(self.position);

        let quat = Quaternion::from(
            Euler {
                x: Deg(self.rotation.x),
                y: Deg(self.rotation.y),
                z: Deg(self.rotation.z),
            }
        );

        let rotate = Matrix4::from(quat);
        let scale = Matrix4::from_nonuniform_scale(self.scale.x, self.scale.y, self.scale.z);

        translate * rotate * scale
    }

    // Get axes
    pub fn forward(&self) -> Vector3 <f32> {
        let matrix = self.matrix();
        let forward = Vector4::new(0.0, 0.0, -1.0, 0.0);
        let forward = matrix * forward;
        Vector3::new(forward.x, forward.y, forward.z)
    }

    pub fn right(&self) -> Vector3 <f32> {
        let matrix = self.matrix();
        let right = Vector4::new(1.0, 0.0, 0.0, 0.0);
        let right = matrix * right;
        Vector3::new(right.x, right.y, right.z)
    }

    pub fn up(&self) -> Vector3 <f32> {
        let matrix = self.matrix();
        let up = Vector4::new(0.0, 1.0, 0.0, 0.0);
        let up = matrix * up;
        Vector3::new(up.x, up.y, up.z)
    }
}
