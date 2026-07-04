extern "C" fn write(fd: c_int, buf: *char, count: c_size_t): c_ssize_t;
extern "C" fn read(fd: c_int, buf: *char, count: c_size_t): c_ssize_t;
extern "C" fn open(path: *char, flags: c_int, mode: c_int): c_int;
extern "C" fn close(fd: c_int): c_int;
extern "C" fn __errno_location(): *c_int;

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

fn print(s: str): void {
    write(1, s.ptr(), s.size() as c_size_t);
}

fn input() {
    var bytesRead = read(0, &buffer as *char, SIZE as c_size_t);
    buffer[bytesRead-1] = '\0';
    return str::from_pointer(&buffer as *char, bytesRead as i64);
}

fn create_file(path: str): c_int {
    return open(path.ptr(), O_CREAT | O_WRONLY | O_EXCL, 0o644);
}

fn write_to_file(fd: c_int, message: str): void {
    var length = (message.size() - 1) as c_size_t;
    write(fd, message.ptr(), length);
}

fn main() {
    print("Enter the file name: ");
    var s = input();
    var fd = create_file(s);
    if fd == -1 {
        var err = *__errno_location();
        if err == 17 {
            print("File already exists.\n");
        } else {
            print("Error creating file");
        }
        return 1;
    }
    
    print("File with name: ");
    print(s);
    print(" will be created.\n");
    print("Write a message: ");
    var message = input();
    write_to_file(fd, message);
    print("Message written to file.\n");
    close(fd);

    return 0;
}