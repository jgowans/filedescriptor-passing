#[macro_use] extern crate nix;

use std::process;
use std::str;
use nix::sys::socket::{MsgFlags, recvmsg};
use std::os::unix::io::RawFd;
use nix::sys::uio::IoVec;

fn main() {
    println!("Hello, world from rust receiver! Pid: {}", process::id());

    let mut buf = [0u8; 50];
    let iov = [IoVec::from_mut_slice(&mut buf[..])];
    let mut cmsgspace = cmsg_space!([RawFd; 1]);
    
    loop {
        let msg = recvmsg(4, &iov, Some(&mut cmsgspace), MsgFlags::empty()).unwrap();
        let msg_txt = str::from_utf8(iov[0].as_slice()).unwrap();
        print!("Rust receiver got: {}\n", msg_txt);
    }
}
