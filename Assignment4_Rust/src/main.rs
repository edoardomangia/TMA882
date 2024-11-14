use clap::Parser;
use ocl::{flags, Buffer, Context, Device, Kernel, Platform, Program, Queue};
use std::error::Error;
use std::fs::File;
use std::io::{BufRead, BufReader};

/// Simple program to perform diffusion simulation using OpenCL
#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// Number of iterations
    #[arg(short = 'n', value_name = "NUM_ITERATIONS")]
    num_iterations: usize,

    /// Diffusion constant
    #[arg(short = 'd', value_name = "DIFFUSION_CONSTANT")]
    diffusion_const: f32,
}

const KERNEL_SRC: &str = r#"
__kernel void diffuse(__global const float* current, __global float* next, 
                      const int width, const int height, const float c) {
    int i = get_global_id(0);
    int j = get_global_id(1);
    // Boundary cells remain 0
    if (i == 0 || j == 0 || i == width-1 || j == height-1) {
        next[j * width + i] = 0.0f;
        return;
    }
    // Calculate index
    int idx = j * width + i;
    // Neighbor indices
    int up = (j-1) * width + i;
    int down = (j+1) * width + i;
    int left = j * width + (i-1);
    int right = j * width + (i+1);
    // Compute average of neighbors
    float avg = (current[up] + current[down] + current[left] + current[right]) / 4.0f;
    // Update temperature
    next[idx] = current[idx] + c * (avg - current[idx]);
}
"#;

fn main() -> Result<(), Box<dyn Error>> {
    // Parse command line arguments
    let args = Args::parse();

    if args.num_iterations == 0 || args.diffusion_const <= 0.0 {
        eprintln!("Invalid arguments. Ensure num_iterations and diffusion_constant are positive.");
        std::process::exit(1);
    }

    // Read input file "init"
    let file = File::open("init").map_err(|e| {
        eprintln!("Failed to open init file: {}", e);
        e
    })?;
    let reader = BufReader::new(file);

    // Read width and height
    let mut lines = reader.lines();
    let first_line = lines
        .next()
        .ok_or("Failed to read width and height from init file.")??;
    let mut dims = first_line.split_whitespace();
    let width: usize = dims
        .next()
        .ok_or("Failed to read width from init file.")?
        .parse()
        .map_err(|_| "Invalid width value.")?;
    let height: usize = dims
        .next()
        .ok_or("Failed to read height from init file.")?
        .parse()
        .map_err(|_| "Invalid height value.")?;

    // Allocate and initialize grids
    let grid_size = width * height;
    let mut current_grid = vec![0.0f32; grid_size];
    let mut next_grid = vec![0.0f32; grid_size];

    // Read initial values
    for line in lines {
        let line = line?;
        let parts: Vec<&str> = line.split_whitespace().collect();
        if parts.len() != 3 {
            continue; // Ignore malformed lines
        }
        let x: usize = parts[0].parse().unwrap_or(usize::MAX);
        let y: usize = parts[1].parse().unwrap_or(usize::MAX);
        let value: f32 = parts[2].parse().unwrap_or(0.0);
        if x < width && y < height {
            current_grid[y * width + x] = value;
        }
    }

    // Set boundary cells to 0
    for i in 0..width {
        current_grid[i] = 0.0; // Top row
        current_grid[(height - 1) * width + i] = 0.0; // Bottom row
    }
    for j in 0..height {
        current_grid[j * width] = 0.0; // Left column
        current_grid[j * width + (width - 1)] = 0.0; // Right column
    }

    // Initialize OpenCL
    // Select the first available GPU device
    let platform = Platform::default();
    let device = Device::first(platform).ok_or("No GPU device found.")?;

    // Create context and queue
    let context = Context::builder()
        .platform(platform)
        .devices(device)
        .build()?;
    let queue = Queue::new(&context, device, None)?;

    // Build program
    let program = Program::builder()
        .src(KERNEL_SRC)
        .devices(device)
        .build(&context)?;

    // Create kernel
    let kernel = Kernel::builder()
        .program(&program)
        .name("diffuse")
        .global_work_size([width, height])
        .arg_named("current", None::<&Buffer<f32>>)
        .arg_named("next", None::<&Buffer<f32>>)
        .arg_scalar(width as i32)
        .arg_scalar(height as i32)
        .arg_scalar(args.diffusion_const)
        .build()?;

    // Create buffers
    let buffer_current = Buffer::<f32>::builder()
        .queue(queue.clone())
        .flags(flags::MEM_READ_WRITE)
        .len(grid_size)
        .copy_host_slice(&current_grid)
        .build()?;
    let buffer_next = Buffer::<f32>::builder()
        .queue(queue.clone())
        .flags(flags::MEM_READ_WRITE)
        .len(grid_size)
        .build()?;

    // Perform iterations
    for iter in 0..args.num_iterations {
        // Set kernel arguments
        let kernel = kernel
            .clone()
            .set_arg("current", &buffer_current)?
            .set_arg("next", &buffer_next)?;

        // Enqueue kernel
        unsafe {
            kernel.enq()?;
        }

        // Swap buffers
        std::mem::swap(&mut buffer_current, &mut buffer_next);
    }

    // Read back the final grid
    buffer_current.read(&mut current_grid).enq()?;

    // Compute average
    let sum: f64 = current_grid.iter().map(|&x| x as f64).sum();
    let average = sum / grid_size as f64;

    // Compute average absolute difference
    let abs_diff_sum: f64 = current_grid
        .iter()
        .map(|&x| (x as f64 - average).abs())
        .sum();
    let avg_abs_diff = abs_diff_sum / grid_size as f64;

    // Output results
    println!("average: {:.6}", average);
    println!("average absolute difference: {:.6}", avg_abs_diff);

    Ok(())
}

