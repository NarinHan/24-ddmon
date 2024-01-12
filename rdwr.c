int 
write_bytes(int fd, void *writing_buf, size_t len)
{
    // return 0 if all given bytes are succesfully sent
    // return 1 otherwise

    char *p = (char *)writing_buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t written ;
        if ((written = write(fd, p, len - acc)) == -1) 
            return 1 ;
        p += written ;
        acc += written ;
    }

    return 0 ;
}

int
read_bytes(int fd, void *reading_buf, size_t len)
{
    // return 0 if all given bytes are succesfully read
    // return 1 otherwise

    char *p = (char *)reading_buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t received ;
        if ((received = read(fd, p, BUF_SIZE)) == -1)
            return 1 ;
        p += received ;
        acc += received ;
    }

    return 0 ;
}