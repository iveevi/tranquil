#[macro_use]
extern crate glium;

use std::fs;
use std::f32::consts::FRAC_PI_2;

// Modules
mod camera;
mod io;
mod transform;

use camera::*;
use io::*;
use transform::*;

use cgmath::Rad;
use cgmath::Vector3;

// Constants
const SAFE_FRAC_PI_2: f32 = FRAC_PI_2 - 0.0001;

#[derive(Copy, Clone)]
struct Vertex {
    position: [f32; 3],
}

implement_vertex!(Vertex, position);

// Mouse cursor state
struct MouseState {
    x: f32,
    y: f32,
    sensitivity: f32,
    drag: bool,
}

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

    let mut mouse_cursor = MouseState {
        x: 0.0,
        y: 0.0,
        sensitivity: 0.005,
        drag: false,
    };

    event_loop.run(move |ev, _, control_flow| {
        // Get delta time
        let now = std::time::Instant::now();
        let delta = now.duration_since(prev).as_secs_f32();
        prev = now;

        // Draw to screen
        let mut target = display.draw();
        target.clear_color(0.0, 0.0, 0.0, 1.0);

        // Transforming the objetcs
        let mut t = Transform::new();
        let time = epoch.elapsed().as_secs_f32();

        t.rotation.y = time * 50.0;
        t.rotation.x = time * 30.0;
        t.rotation.z = time * 10.0;

        // Drawing
        let model : [[f32; 4]; 4] = t.matrix().into();
        let view : [[f32; 4]; 4] = camera.view().into();
        let perspective : [[f32; 4]; 4] = camera.perspective().into();

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

        // Camera movement
        let speed = 5.0 * delta;

        let forward = camera.forward();
        let right = camera.right();

        if keyboard.is_pressed(VirtualKeyCode::W) {
            camera.position += forward * speed;
        } else if keyboard.is_pressed(VirtualKeyCode::S) {
            camera.position -= forward * speed;
        }

        if keyboard.is_pressed(VirtualKeyCode::A) {
            camera.position -= right * speed;
        } else if keyboard.is_pressed(VirtualKeyCode::D) {
            camera.position += right * speed;
        }

        if keyboard.is_pressed(VirtualKeyCode::Q) {
            camera.position -= speed * Vector3::unit_y();
        } else if keyboard.is_pressed(VirtualKeyCode::E) {
            camera.position += speed * Vector3::unit_y();
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

                        camera.yaw += Rad(dx * sensitivity);
                        camera.pitch -= Rad(dy * sensitivity);

                        if camera.pitch > Rad(SAFE_FRAC_PI_2) {
                            camera.pitch = Rad(SAFE_FRAC_PI_2);
                        } else if camera.pitch < Rad(-SAFE_FRAC_PI_2) {
                            camera.pitch = Rad(-SAFE_FRAC_PI_2);
                        }
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
