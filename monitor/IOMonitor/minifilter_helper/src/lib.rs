#![no_std]
#![feature(alloc_error_handler)]
#![feature(lang_items)]
#![feature(core_ffi_c)]
#![feature(core_c_str)]
extern crate alloc;

pub mod allocator;
pub mod header;
pub mod common;

use alloc::string::{String, ToString};
use alloc::{format};
use alloc::alloc::{alloc, dealloc};
use core::alloc::Layout;
use core::panic::PanicInfo;
use core::ptr::{null_mut, slice_from_raw_parts};
use core::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use core::mem::size_of;
use libm::log2;

use common::*;

// use allocator::*;
use crate::header::WCHAR;

const MAX_STRING_LENGTH: usize = 512;
const MAX_RECORDS_SIZE: usize = 1000;
const SIZE_FOR_FLUSH: usize = 800;

const MAX_STRINGIFY_ALLOCATE_LENGTH: usize = MAX_RECORDS_SIZE * size_of::<Record>() * 5;

static mut RECORDS: *mut Records = null_mut();
static mut STRINGIFY_FLUSH_STR: *mut u8 = null_mut();
static mut WRITE_LOG_FILE_LOCK: *mut AtomicBool = null_mut();

#[global_allocator]
static ALLOCATOR: allocator::KernelAllocator = allocator::KernelAllocator::new(
    u32::from_ne_bytes(*b"rust")
);


#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

// -- just add them to fix some compilation errors, not sure if they have side-effect
#[no_mangle]
pub extern "C" fn __CxxFrameHandler3() {}

#[no_mangle]
static _fltused: i32 = 0;
// --

#[allow(unused)]
#[no_mangle]
pub extern "C" fn __helper_log2(p: f64) -> f64 {
    // thanks to libm::log2
    // the core::intrinsic::log2f64 cannot work!
    log2(p)
}

#[allow(unused)]
#[no_mangle]
pub extern "C" fn __helper_is_blacklist(process_name_: *mut WCHAR, process_name_length: u64) -> u8 {
    let proc_name_slice = slice_from_raw_parts(process_name_,
                                               process_name_length as _);
    if is_proc_in_blacklist(unsafe { proc_name_slice.as_ref().unwrap() },
                            process_name_length as _) {
        return 1;
    } else {
        return 0;
    }
}

#[allow(unused)]
#[no_mangle]
pub extern "C" fn __helper_dealloc_all_resource() {
    unsafe {
        if !RECORDS.is_null() {
            let record_layout = Layout::new::<Records>();
            dealloc(RECORDS as *mut u8, record_layout);
        }
        if !STRINGIFY_FLUSH_STR.is_null() {
            let flush_str_layout = Layout::new::<[u8; MAX_STRINGIFY_ALLOCATE_LENGTH]>();
            dealloc(STRINGIFY_FLUSH_STR, flush_str_layout);
        }
        if !WRITE_LOG_FILE_LOCK.is_null() {
            let log_layout = Layout::new::<AtomicBool>();
            dealloc(WRITE_LOG_FILE_LOCK as *mut u8, log_layout);
        }
    }
}

#[allow(unused)]
#[no_mangle]
pub extern "C" fn __helper_convert_float_to_str(f: f64, s: *mut u8, s_size: u64) -> u8 {
    let f_str = f.to_string();
    let bytes = f_str.as_bytes();
    let size = s_size as usize;
    if size < bytes.len() + 1 {
        return 0;
    }
    unsafe {
        let mut i = 0usize;
        for c in bytes {
            let p = s.add(i);
            *p = *c;
            i += 1;
        }
        let p = s.add(i);
        *p = 0;
    }
    return 1;
}

// #[allow(unused)]
// #[no_mangle]
// pub extern "C" fn __helper_output_header_and_copy_buf(input_buf: *mut *mut u8, size: *mut u64) -> u8 {
//     let s = Record::stringify_header();
//     unsafe {
//         if (*input_buf).is_null() {
//             return 0;
//         }
//         let is_copy_ok = copy_string_to_buffer(s, input_buf, size);
//         return if is_copy_ok {
//             1
//         } else {
//             255
//         };
//     }
// }

#[allow(unused)]
#[no_mangle]
pub extern "C" fn __helper_add_record(
    time: u64,
    major_opr_: u8,
    minor_opr_: u8,
    process_name_: *mut WCHAR,
    process_name_length: u64,
    pid: u64, tid: u64,
    parent_pid: u64,
    buffer_length: u64,
    entropy_bit_based: f64,
    entropy_byte_based: f64,
    file_name_: *mut WCHAR,
    file_name_length: u64,
    is_rename: u8,
    is_delete: u8,
    file_size: u64,
    offset: u64) -> u8 {
    let major_opr = OperationType::from_u8(major_opr_);
    let minor_opr = OperationType::from_u8(minor_opr_);
    let mut process_name: [WCHAR; MAX_STRING_LENGTH] = [0; MAX_STRING_LENGTH];
    let mut file_name: [WCHAR; MAX_STRING_LENGTH] = [0; MAX_STRING_LENGTH];
    unsafe {
        let mut p: *mut WCHAR = process_name_;
        for i in 0..process_name_length as usize {
            process_name[i] = *p;
            p = p.add(1);
        }

        p = file_name_;
        for i in 0..file_name_length as usize {
            file_name[i] = *p;
            p = p.add(1);
        }
    }
    let record = Record {
        time,
        major_opr,
        minor_opr,
        process_name,
        process_name_length,
        pid,
        tid,
        parent_pid,
        buffer_length,
        entropy_bit_based,
        entropy_byte_based,
        file_name,
        file_name_length,
        is_rename,
        is_delete,
        file_size,
        offset,
    };
    unsafe {
        if RECORDS.is_null() {
            let layout = Layout::new::<Records>();
            RECORDS = alloc(layout) as *mut Records;
            (*RECORDS).initialize();
        }

        let need_flush = (*RECORDS).add_a_record(record);
        return if need_flush {
            1
        } else {
            0
        };
    }
}

#[allow(unused)]
#[no_mangle]
pub extern "C" fn __helper_flush_and_copy_buf(input_buf: *mut *mut u8, size: *mut u64) -> u8 {
    let res = unsafe { (*RECORDS).flush() };
    if res.is_none() {
        return 0;
    }
    unsafe {
        if STRINGIFY_FLUSH_STR.is_null() {
            let layout = Layout::new::<[u8; MAX_STRINGIFY_ALLOCATE_LENGTH]>();
            STRINGIFY_FLUSH_STR = alloc(layout) as *mut u8;
        }
    }
    let s = res.unwrap();

    unsafe { *input_buf = STRINGIFY_FLUSH_STR };
    let is_copy_ok = unsafe { copy_string_to_buffer(s, input_buf, size, MAX_STRINGIFY_ALLOCATE_LENGTH) };
    return if is_copy_ok {
        1
    } else {
        255
    };
}

#[allow(unused)]
#[no_mangle]
pub extern "C" fn __helper_log_file_lock() {
    unsafe {
        if WRITE_LOG_FILE_LOCK.is_null() {
            let layout = Layout::new::<AtomicBool>();
            WRITE_LOG_FILE_LOCK = alloc(layout) as *mut AtomicBool;
            (*WRITE_LOG_FILE_LOCK) = AtomicBool::new(true);
        }
        while (*WRITE_LOG_FILE_LOCK).compare_exchange(true,
                                                      false,
                                                      Ordering::SeqCst,
                                                      Ordering::SeqCst).is_ok() {}
    }
}

#[allow(unused)]
#[no_mangle]
pub extern "C" fn __helper_log_file_unlock() {
    unsafe {
        // if WRITE_LOG_FILE_LOCK.is_null() {
        //     let layout = Layout::new::<AtomicBool>();
        //     WRITE_LOG_FILE_LOCK = alloc(layout) as *mut AtomicBool;
        // }
        while (*WRITE_LOG_FILE_LOCK).compare_exchange(false,
                                                      true,
                                                      Ordering::SeqCst,
                                                      Ordering::SeqCst).is_ok() {}
    }
}

#[derive(Debug, Clone, Copy)]
struct Record {
    // 100-nanosecond intervals since January 1, 1601
    // in line with https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kequerysystemtime-r1
    time: u64,
    major_opr: OperationType,
    minor_opr: OperationType,
    process_name: [WCHAR; MAX_STRING_LENGTH],
    process_name_length: u64,
    pid: u64,
    tid: u64,
    parent_pid: u64,
    buffer_length: u64,
    entropy_bit_based: f64,
    entropy_byte_based: f64,
    file_name: [WCHAR; MAX_STRING_LENGTH],
    file_name_length: u64,
    is_rename: u8,
    is_delete: u8,
    file_size: u64,
    offset: u64,
}

impl Default for Record {
    fn default() -> Self {
        Self {
            time: 0,
            major_opr: OperationType::default(),
            minor_opr: OperationType::default(),
            process_name: [0; MAX_STRING_LENGTH],
            process_name_length: 0,
            pid: 0,
            tid: 0,
            parent_pid: 0,
            buffer_length: 0,
            entropy_bit_based: 0.0,
            entropy_byte_based: 0.0,
            file_name: [0; MAX_STRING_LENGTH],
            file_name_length: 0,
            is_rename: 0,
            is_delete: 0,
            file_size: 0,
            offset: 64,
        }
    }
}

impl Record {
    pub fn stringify(&self) -> String {
        let minor_opr = if self.minor_opr == OperationType::IRP_MJ_CREATE {
            OperationType::OP_NOTHING
        } else {
            self.minor_opr
        };
        format!("{}\t{:?}\t{:?}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\n",
                self.time,
                self.major_opr,
                minor_opr,
                String::from_utf16(&self.process_name[0..self.process_name_length as usize]).unwrap(),
                self.pid, self.tid, self.parent_pid,
                self.buffer_length,
                self.entropy_bit_based,
                self.entropy_byte_based,
                String::from_utf16(&self.file_name[0..self.file_name_length as usize]).unwrap(),
                self.is_rename,
                self.is_delete,
                self.file_size,
                self.offset)
    }

    pub fn stringify_header() -> String {
        format!("time\tmajor_opr\tminor_opr\tprocess_name\tpid\ttid\tparent_pid\tbuffer_length\tentropy_bit_based\tentropy_byte_based\tfile_name\tis_rename\tis_delete\tfile_size\toffset\n")
    }
}

#[derive(Debug)]
struct Records {
    // usable range: [0, index)
    records: [Record; MAX_RECORDS_SIZE],
    index: AtomicUsize,
    is_first: bool,
    flush_lock: AtomicBool,
}

impl Default for Records {
    fn default() -> Self {
        Self {
            records: [Record::default(); MAX_RECORDS_SIZE],
            index: AtomicUsize::new(0),
            flush_lock: AtomicBool::new(false),
            is_first: true,
        }
    }
}

impl Records {
    pub fn initialize(&mut self) {
        self.records = [Record::default(); MAX_RECORDS_SIZE];
        self.index = AtomicUsize::new(0);
        self.flush_lock = AtomicBool::new(false);
        self.is_first = true;
    }
    pub fn stringify(&mut self) -> String {
        let mut res = String::new();
        if self.is_first {
            let header = Record::stringify_header();
            res += &header;
            self.is_first = false;
        }
        let index = self.index.load(Ordering::SeqCst);
        for i in 0..index {
            res += &self.records[i].stringify();
        }
        return res;
    }

    /// Note!: return true if it needs flush
    pub fn add_a_record(&mut self, record: Record) -> bool {
        // wait if it needs flush
        self.wait_if_flush();

        // atomic read and add
        let index = self.index.fetch_add(1, Ordering::SeqCst);
        if index >= MAX_RECORDS_SIZE {
            return true;
        }
        self.records[index] = record;
        return index > SIZE_FOR_FLUSH;
    }

    pub fn flush(&mut self) -> Option<String> {
        // lock the flush lock so that others cannot add record or flush it
        self.lock_flush();

        // we don't need to flush it
        if self.index.load(Ordering::SeqCst) < SIZE_FOR_FLUSH {
            return None;
        }

        let str = self.stringify();
        self.index.store(0, Ordering::SeqCst);
        self.unlock_flush();
        return Some(str);
    }

    fn lock_flush(&mut self) {
        while self.flush_lock.compare_exchange(false,
                                               true,
                                               Ordering::SeqCst,
                                               Ordering::SeqCst).is_ok() {}
    }

    fn unlock_flush(&mut self) {
        while self.flush_lock.compare_exchange(true,
                                               false,
                                               Ordering::SeqCst,
                                               Ordering::SeqCst).is_ok() {}
    }

    fn wait_if_flush(&self) {
        while self.flush_lock.load(Ordering::SeqCst) {}
    }
}

#[allow(unused)]
fn is_proc_in_blacklist(proc_name: &[WCHAR], proc_name_length: usize) -> bool {
    // let s = format!("{}",
    //                 String::from_utf16(&proc_name[0..proc_name_length]).unwrap());
    // for var in PROC_BLACKLIST {
    //     if s.contains(var) {
    //         return true;
    //     }
    // }
    return false;
}

unsafe fn copy_string_to_buffer(s: String, input_buf: *mut *mut u8, size: *mut u64, max_length: usize) -> bool {
    let buffer_content = *input_buf;

    let mut index = 0usize;
    for wchar in s.bytes() {
        if index >= max_length {
            return false;
        }
        let p = buffer_content.add(index);
        *p = wchar;
        index += 1;
    }
    *size = index as _;
    let p = buffer_content.add(index);
    *p = 0;

    return true;
}