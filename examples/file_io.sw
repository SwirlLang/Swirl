extern "C" fn write(fd: c_int, buf: *char, count: c_size_t): c_ssize_t;
extern "C" fn read(fd: c_int, buf: *char, count: c_size_t): c_ssize_t;
extern "C" fn open(path: *char, flags: c_int, mode: c_int): c_int;
extern "C" fn close(fd: c_int): c_int;
extern "C" fn __errno_location(): *c_int;
extern "C" fn calloc(num: c_size_t, size: c_size_t): *void;
extern "C" fn free(ptr: *void): void;
extern "C" fn memcpy(dest: *void, src: *void, count: c_size_t)

// fs flags
comptime let O_CREAT = 0o100;
comptime let O_WRONLY = 0o1;
comptime let O_TRUNC = 0o1000;
comptime let O_EXCL = 0o200;
comptime let O_RDWR = 0o2;
comptime let O_APPEND = 0o2000;
comptime let O_RDONLY = 0o0;
comptime let O_NONBLOCK = 0o4000;
comptime let O_SYNC = 0o10000;

comptime let SIZE = 4096;
var buffer: [char | SIZE];

struct BufferedWriter {
    var fd: c_int;
    var pos: i32;
    var buf: *char;

    export fn open(path: str) {
        var tmp: BufferedWriter;
        tmp.pos = 0;
        tmp.buf = calloc(SIZE, 1) as *char;
        tmp.fd = open(path.ptr(), O_CREAT | O_WRONLY | O_TRUNC, 0o644);
        return tmp;
    }

    fn flush(&self): void {
        write(self.fd, self.buf, self.pos as c_size_t);
        self.pos = 0;
    }

    fn write(&self, data: str): void {
        var bytes_written = 0;
    
        while bytes_written < data.size() {
            var available_space = SIZE - self.pos;
            var remaining_data = data.size() - bytes_written;
            
            var amt_to_copy = remaining_data;
            if amt_to_copy > available_space {
                amt_to_copy = available_space;
            }
            
            var dest = (self.buf as i64 + self.pos) as *void; 
            var src = (data.ptr() as i64 + bytes_written) as *void;
            memcpy(dest, src, amt_to_copy);
            
            self.pos += amt_to_copy;
            bytes_written += amt_to_copy;
            
            if self.pos == SIZE {
                self.flush();
            }
        }
    }
    
    fn close(&self): void {
        self.flush();
        close(self.fd);
        free(self.buf as *void);
    }
}


fn print(s: str): void {
    write(1, s.ptr(), s.size() as c_size_t);
}

fn input() {
    var bytesRead = read(0, &buffer as *char, SIZE as c_size_t);
    buffer[bytesRead - 1] = '\0';
    return str::from_pointer(&buffer as *char, bytesRead as i64);
}

fn create_file(path: str): c_int {
    let fd = open(path.ptr(), O_CREAT | O_WRONLY | O_EXCL, 0o644);
    if fd < 0 {
        if *__errno_location() == 17 { // EEXIST
            print("File already exists\n");
        } else {
            print("Error creating file\n");
        }
    }
    return fd;
}

fn write_to_file(fd: c_int, message: str): void {
    var length = (message.size() - 1) as c_size_t;
    write(fd, message.ptr(), length);
}

fn main() {
    print("Enter the file name: ");
    var file = input();
    let writer = BufferedWriter::open(file);

    writer.write("Hello, World!\n");
    writer.write("Swirl, World!\n");
    writer.write("No Mans, World!\n");
    writer.close();
    
    print("Messages written to file.\n");

    return 0;
}