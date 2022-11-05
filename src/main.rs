use std::fs;

#[macro_use]
extern crate glium;

// Modules
mod io;

use cgmath::InnerSpace;
use io::*;

#[derive(Copy, Clone)]
struct Vertex {
    position: [f32; 3],
}

implement_vertex!(Vertex, position);

use cgmath::Vector3;
use cgmath::Vector4;
use cgmath::Matrix4;
use cgmath::Rad;
use cgmath::Point3;
use cgmath::PerspectiveFov;
use cgmath::Euler;
use cgmath::Quaternion;
use cgmath::Deg;

#[derive(Copy, Clone)]
struct Transform {
    position: Vector3 <f32>,
    rotation: Vector3 <f32>, // In degrees
    scale: Vector3 <f32>,
}

impl Transform {
    fn new() -> Transform {
        Transform {
            position: Vector3::new(0.0, 0.0, 0.0),
            rotation: Vector3::new(0.0, 0.0, 0.0),
            scale: Vector3::new(1.0, 1.0, 1.0),
        }
    }

    fn matrix(&self) -> Matrix4 <f32> {
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
    fn forward(&self) -> Vector3 <f32> {
        let matrix = self.matrix();
        let forward = Vector4::new(0.0, 0.0, -1.0, 0.0);
        let forward = matrix * forward;
        Vector3::new(forward.x, forward.y, forward.z)
    }

    fn right(&self) -> Vector3 <f32> {
        let matrix = self.matrix();
        let right = Vector4::new(1.0, 0.0, 0.0, 0.0);
        let right = matrix * right;
        Vector3::new(right.x, right.y, right.z)
    }

    fn up(&self) -> Vector3 <f32> {
        let matrix = self.matrix();
        let up = Vector4::new(0.0, 1.0, 0.0, 0.0);
        let up = matrix * up;
        Vector3::new(up.x, up.y, up.z)
    }
}

// Camera
#[derive(Copy, Clone, Debug)]
struct Camera {
    fov: f32
}

impl Camera {
    // Constructor
    fn new() -> Camera {
        Camera {
            fov: 90.0
        }
    }

    fn perspective(&self) -> Matrix4 <f32> {
        PerspectiveFov {
            fovy: Rad(self.fov.to_radians()),
            aspect: 1.0,
            near: 0.1,
            far: 100.0,
        }.into()
    }
}

// Mouse cursor state
struct MouseGimbal {
    x: f32,
    y: f32,
    sensitivity: f32,
    yaw: Rad <f32>,
    pitch: Rad <f32>,
    drag: bool,
}

use std::f32::consts::FRAC_PI_2;

const SAFE_FRAC_PI_2: f32 = FRAC_PI_2 - 0.0001;

fn main()
{
    let mut camera = Camera::new();
    let mut transform = Transform::new();

    // Read vertex and fragment shaders
    let vertex_shader_src = fs::read_to_string("src/base.vert")
        .expect("Failed to read vertex shader");

    let fragment_shader_src = fs::read_to_string("src/base.frag")
        .expect("Failed to read fragment shader");

    // Setup Glium
    use glium::{glutin, Surface};
    use glium::glutin::event::VirtualKeyCode;

    let mut event_loop = glutin::event_loop::EventLoop::new();
    let wb = glutin::window::WindowBuilder::new();
    let cb = glutin::ContextBuilder::new();
    let display = glium::Display::new(wb, cb, &event_loop).unwrap();

    // Event handlers
    let mut keyboard = Keyboard::new();
    
    // Testing mesh
    let vertex1 = Vertex { position: [-0.5, -0.5, 0.0] };
    let vertex2 = Vertex { position: [ 0.0,  0.5, 0.0] };
    let vertex3 = Vertex { position: [ 0.5, -0.25, 0.0] };

    let mut shape = vec![vertex1, vertex2, vertex3];

    // Add offset triangles
    for i in 0..10 {
        let vertex1 = Vertex { position: [-0.5, -0.5, -i as f32] };
        let vertex2 = Vertex { position: [ 0.0,  0.5, -i as f32] };
        let vertex3 = Vertex { position: [ 0.5, -0.25, -i as f32] };

        shape.push(vertex1);
        shape.push(vertex2);
        shape.push(vertex3);
    }

    let vertex_buffer = glium::VertexBuffer::new(&display, &shape).unwrap();
    let indices = glium::index::NoIndices(glium::index::PrimitiveType::TrianglesList);

    // Load shaders
    let program = glium::Program::from_source(
        &display, &vertex_shader_src,
        &fragment_shader_src, None
    ).unwrap();

    let epoch = std::time::Instant::now();
    let mut prev = std::time::Instant::now();

    let mut mouse_cursor = MouseGimbal {
        x: 0.0,
        y: 0.0,
        sensitivity: 0.005,
        yaw: Rad(FRAC_PI_2),
        pitch: Rad(0.0),
        drag: false,
    };

    event_loop.run(move |ev, _, control_flow| {
        // Draw to screen
        let mut target = display.draw();
        target.clear_color(0.0, 0.0, 0.0, 1.0);

        let mut t = Transform::new();

        let model : [[f32; 4]; 4] = t.matrix().into();
        let perspective : [[f32; 4]; 4] = camera.perspective().into();

        let pos = Point3::new(transform.position.x, transform.position.y, transform.position.z);
        
        let yaw = mouse_cursor.yaw;
        let pitch = mouse_cursor.pitch;

        let view : [[f32; 4]; 4] = Matrix4::look_to_rh(
            pos,
            Vector3::new(
                yaw.0.cos(),
                pitch.0.sin(),
                yaw.0.sin(),
            ).normalize(),
            Vector3::unit_y(),
        ).into();

        let uniforms = uniform! {
            model: model,
            view: view,
            projection: perspective,
        };

        target.draw(
            &vertex_buffer, &indices, &program,
            &uniforms, &Default::default()
        ).unwrap();

        target.finish().unwrap();

        // Get delta time
        let now = std::time::Instant::now();
        let delta = now.duration_since(prev).as_secs_f32();
        prev = now;

        // Camera movement
        let speed = 5.0 * delta;

        // let up = transform.up();

        let (yaw_sin, yaw_cos) = yaw.0.sin_cos();
        let forward = Vector3::new(yaw_cos, 0.0, yaw_sin).normalize();
        let right = Vector3::new(-yaw_sin, 0.0, yaw_cos).normalize();

        if keyboard.is_pressed(VirtualKeyCode::W) {
            transform.position += forward * speed;
        } else if keyboard.is_pressed(VirtualKeyCode::S) {
            transform.position -= forward * speed;
        }

        if keyboard.is_pressed(VirtualKeyCode::A) {
            transform.position -= right * speed;
        } else if keyboard.is_pressed(VirtualKeyCode::D) {
            transform.position += right * speed;
        }

        if keyboard.is_pressed(VirtualKeyCode::Q) {
            transform.position -= speed * Vector3::unit_y();
        } else if keyboard.is_pressed(VirtualKeyCode::E) {
            transform.position += speed * Vector3::unit_y();
        }

        // Schedule a redraw
        let next_frame_time = std::time::Instant::now()
            + std::time::Duration::from_nanos(16_666_667);

        *control_flow = glutin::event_loop::ControlFlow::WaitUntil(next_frame_time);

        // Manage events
        match ev {
            glutin::event::Event::WindowEvent { event, .. } => match event {
                // Close requests
                glutin::event::WindowEvent::CloseRequested => {
                    *control_flow = glutin::event_loop::ControlFlow::Exit;
                    return;
                },

                // Keyboard events
                glutin::event::WindowEvent::KeyboardInput { input, .. } => {
                    keyboard.update(input.virtual_keycode.unwrap(), input.state);
                },

                // Mouse events
                glutin::event::WindowEvent::MouseInput { state, button, .. } => {
                    if button == glutin::event::MouseButton::Left {
                        mouse_cursor.drag = state == glutin::event::ElementState::Pressed;
                    }
                },

                glutin::event::WindowEvent::CursorMoved { position, .. } => {
                    let x : f32 = position.x as f32;
                    let y : f32 = position.y as f32;

                    if mouse_cursor.drag {
                        let dx : f32 = x - mouse_cursor.x;
                        let dy : f32 = y - mouse_cursor.y;

                        let sensitivity = mouse_cursor.sensitivity;

                        mouse_cursor.yaw += Rad(dx * sensitivity);
                        mouse_cursor.pitch -= Rad(dy * sensitivity);

                        if mouse_cursor.pitch > Rad(SAFE_FRAC_PI_2) {
                            mouse_cursor.pitch = Rad(SAFE_FRAC_PI_2);
                        } else if mouse_cursor.pitch < Rad(-SAFE_FRAC_PI_2) {
                            mouse_cursor.pitch = Rad(-SAFE_FRAC_PI_2);
                        }

                        println!("Yaw: {}, Pitch: {}", mouse_cursor.yaw.0, mouse_cursor.pitch.0);
                    } 

                    mouse_cursor.x = x;
                    mouse_cursor.y = y;
                },

                // Other events
                _ => return,
            },
            _ => (),
        }
    });
}
