export struct str {
    var __Sw_buffer: *char;
    var __Sw_length: i64;

    export fn size(&self):  i64  { return self.__Sw_length; }
    export fn ptr (&self): *char { return self.__Sw_buffer; }

    export fn from_pointer(buf: *char, len: i64) {
        var inst: str;
        inst.__Sw_buffer = buf;
        inst.__Sw_length = len;
        return inst;
    }
}