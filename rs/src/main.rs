use std::process::{Command, Stdio};

fn main() {
    println!("Hello, world!");

    let mut _sender = Command::new("./sender")
	//.stdin(Stdio::piped())
	//.stdout(Stdio::piped())
	.spawn()
	.expect("failed to execute child");

    let mut _receiver = Command::new("./receiver")
	//.stdin(Stdio::piped())
	//.stdout(Stdio::piped())
	.spawn()
	.expect("failed to execute child");

    std::thread::sleep(std::time::Duration::from_secs(100));
}
