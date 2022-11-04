use std::fs;

#[macro_use]
extern crate glium;

// Modules
mod math;

use math::*;

#[derive(Copy, Clone)]
struct Vertex {
    position: [f32; 3],
}

implement_vertex!(Vertex, position);

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
    let mut camera = Camera::new();
    println!("Camera: {:?}", camera);
    println!("View: {:?}", camera.view());
    println!("Perspective: {:?}", camera.perspective());

    // Read vertex and fragment shaders
    let vertex_shader_src = fs::read_to_string("src/base.vert")
        .expect("Failed to read vertex shader");

    let fragment_shader_src = fs::read_to_string("src/base.frag")
        .expect("Failed to read fragment shader");

    // Setup Glium
    use glium::{glutin, Surface};

    let mut event_loop = glutin::event_loop::EventLoop::new();
    let wb = glutin::window::WindowBuilder::new();
    let cb = glutin::ContextBuilder::new();
    let display = glium::Display::new(wb, cb, &event_loop).unwrap();

    // Testing mesh
    let vertex1 = Vertex { position: [-0.5, -0.5, 5.0] };
    let vertex2 = Vertex { position: [ 0.0,  0.5, 5.0] };
    let vertex3 = Vertex { position: [ 0.5, -0.25, 5.0] };

    let mut shape = vec![vertex1, vertex2, vertex3];

    // Add offset triangles
    for i in 0..10 {
        let vertex1 = Vertex { position: [-0.5, -0.5, 5.0 + i as f32] };
        let vertex2 = Vertex { position: [ 0.0,  0.5, 5.0 + i as f32] };
        let vertex3 = Vertex { position: [ 0.5, -0.25, 5.0 + i as f32] };

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

    event_loop.run(move |ev, _, control_flow| {
        // Draw to screen
        let mut target = display.draw();
        target.clear_color(0.0, 0.0, 1.0, 1.0);

        let uniforms = uniform! {
            model: Matrix::identity().data,
            view: camera.view().data,
            projection: camera.perspective().data,
        };

        target.draw(
            &vertex_buffer, &indices, &program,
            &uniforms, &Default::default()
        ).unwrap();

        target.finish().unwrap();

        // Move camera in a circle
        let t = std::time::Instant::now();
        let t = t.duration_since(epoch).as_secs_f32();
        let x = t.cos() * 5.0;
        let z = t.sin() * 5.0;

        camera.transform.data[0][3] = x;
        camera.transform.data[2][3] = z;

        println!("\nt = {}, x = {}, z = {}", t, x, z);
        println!("\tCamera: {:?}", camera.transform.position());

        // Schedule a redraw
        let next_frame_time = std::time::Instant::now()
            + std::time::Duration::from_nanos(16_666_667);

        *control_flow = glutin::event_loop::ControlFlow::WaitUntil(next_frame_time);

        // Manage events
        match ev {
            glutin::event::Event::WindowEvent { event, .. } => match event {
                glutin::event::WindowEvent::CloseRequested => {
                    *control_flow = glutin::event_loop::ControlFlow::Exit;
                    return;
                },

                glutin::event::WindowEvent::KeyboardInput { input, .. } => {
                    if let Some(glutin::event::VirtualKeyCode::Escape) = input.virtual_keycode {
                        *control_flow = glutin::event_loop::ControlFlow::Exit;
                        return;
                    }
                },

                _ => return,
            },
            _ => (),
        }
    });
}
