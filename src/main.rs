#[macro_use]
extern crate glium;

use std::fs;
use std::f32::consts::FRAC_PI_2;

// Modules
mod camera;
mod io;
mod transform;

use camera::*;
use glium::texture::Texture2dDataSink;
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

// Manage camera motion with keyboard
fn camera_keyboard_movement(camera: &mut Camera, keyboard: &Keyboard, delta_time: f32) {
    use glium::glutin::event::VirtualKeyCode;

    let speed = 5.0 * delta_time;

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
}

// Manage camera motion with mouse
fn camera_mouse_movement(camera: &mut Camera, state: &mut MouseState, position: (f32, f32)) {
    let x : f32 = position.0 as f32;
    let y : f32 = position.1 as f32;

    if state.drag {
        let dx : f32 = x - state.x;
        let dy : f32 = y - state.y;

        let sensitivity = state.sensitivity;

        camera.yaw += Rad(dx * sensitivity);
        camera.pitch -= Rad(dy * sensitivity);

        if camera.pitch > Rad(SAFE_FRAC_PI_2) {
            camera.pitch = Rad(SAFE_FRAC_PI_2);
        } else if camera.pitch < Rad(-SAFE_FRAC_PI_2) {
            camera.pitch = Rad(-SAFE_FRAC_PI_2);
        }
    } 

    state.x = x;
    state.y = y;
}

fn main() {
    /* use noise::{NoiseFn, Perlin, Fbm};

    let perlin = Perlin::new(0);
    let fbm : Fbm <Perlin> = Fbm::new(0);

    // Save to file...
    let n = 1024;
    let mut data : Vec <u8> = Vec::with_capacity(n * n);

    let freq = 32.0;
    for i in 0..n {
        for j in 0..n {
            let x = freq * (i as f64 / n as f64);
            let y = freq * (j as f64 / n as f64);

            let value = fbm.get([x, y]);

            let value = (value + 1.0) / 2.0;

            let value = (value * 255.0) as u8;

            data.push(value);
        }
    }

    println!("Saving to file, data = {:?}", data);

    // Save as image
    use image::ImageBuffer;

    let img : ImageBuffer <image::Luma<u8>, _> = ImageBuffer::from_vec(n as u32, n as u32, data).unwrap();
    img.save("perlin.png").unwrap();

    return; */

    let mut camera = Camera::new();

    // Setup Glium
    use glium::{glutin, Surface};

    let mut event_loop = glutin::event_loop::EventLoop::new();
    let wb = glutin::window::WindowBuilder::new();
    let cb = glutin::ContextBuilder::new();
    let display = glium::Display::new(wb, cb, &event_loop).unwrap();

    // Event handlers
    let mut keyboard = Keyboard::new();
    
    // Testing mesh (square)
    let vertex1 = Vertex { position: [-10.0, 0.0, -10.0] };
    let vertex2 = Vertex { position: [ 10.0, 0.0, -10.0] };
    let vertex3 = Vertex { position: [ 10.0, 0.0,  10.0] };
    let vertex4 = Vertex { position: [-10.0, 0.0,  10.0] };

    let mut shape = vec![vertex1, vertex2, vertex3, vertex1, vertex3, vertex4];

    let vertex_buffer = glium::VertexBuffer::new(&display, &shape).unwrap();
    let indices = glium::index::NoIndices(
        glium::index::PrimitiveType::Patches {
            vertices_per_patch: 3
        }
    );

    // Load textures
    let path = "perlin.png";

    let image = image::open(path).unwrap().into_luma8();
    let image_dimensions = image.dimensions();
    let raw_image : Vec <u8> = image.into_raw();

    // CoW
    use std::borrow::Cow;

    let texture = glium::texture::RawImage2d::from_raw(
        Cow::Owned(raw_image),
        image_dimensions.0,
        image_dimensions.1,
    );

    let texture = glium::texture::Texture2d::new(&display, texture).unwrap();

    // Load shaders
    let program = glium::Program::new(
        &display,
        glium::program::SourceCode {
            vertex_shader: &fs::read_to_string("src/base.vert").unwrap(),
            tessellation_control_shader: Some(&fs::read_to_string("src/heightmap.tcs").unwrap()),
            tessellation_evaluation_shader: Some(&fs::read_to_string("src/heightmap.tes").unwrap()),
            geometry_shader: None,
            fragment_shader: &fs::read_to_string("src/base.frag").unwrap(),
        }
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
        target.clear_color_and_depth((0.0, 0.0, 0.0, 1.0), 1.0);

        // Drawing
        let model: [[f32; 4]; 4] = Transform::new().matrix().into();
        let view: [[f32; 4]; 4] = camera.view().into();
        let perspective: [[f32; 4]; 4] = camera.perspective().into();

        let uniforms = uniform! {
            model: model,
            view: view,
            projection: perspective,
            heightmap: &texture
        };

        let params = glium::DrawParameters {
            depth: glium::Depth {
                test: glium::DepthTest::IfLessOrEqual,
                write: true,
                ..Default::default()
            },
            polygon_mode: glium::draw_parameters::PolygonMode::Fill,
            ..Default::default()
        };

        target.draw(
            &vertex_buffer, &indices, &program,
            &uniforms, &params
        ).unwrap();

        target.finish().unwrap();

        // Camera movement
        camera_keyboard_movement(&mut camera, &keyboard, delta);

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
                    camera_mouse_movement(
                        &mut camera, &mut mouse_cursor,
                        (position.x as f32, position.y as f32)
                    );
                },

                // Other events
                _ => return,
            },
            _ => (),
        }
    });
}
