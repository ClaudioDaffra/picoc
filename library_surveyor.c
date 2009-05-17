#include "picoc.h"

static int Blobcnt, Blobx1, Blobx2, Bloby1, Bloby2, Iy1, Iy2, Iu1, Iu2, Iv1, Iv2;
static int GPSlatdeg, GPSlatmin, GPSlondeg, GPSlonmin, GPSalt, GPSfix, GPSsat, GPSutc;
static int ScanVect[16];

void PlatformLibraryInit()
{
    struct ValueType *IntArrayType;
    
    IntArrayType = TypeGetMatching(NULL, &IntType, TypeArray, 16, NULL);
    VariableDefinePlatformVar(NULL, "scanvect", IntArrayType, (union AnyValue *)&ScanVect, FALSE);
    VariableDefinePlatformVar(NULL, "blobcnt", &IntType, (union AnyValue *)&Blobcnt, FALSE);
    VariableDefinePlatformVar(NULL, "blobx1", &IntType, (union AnyValue *)&Blobx1, FALSE);
    VariableDefinePlatformVar(NULL, "blobx2", &IntType, (union AnyValue *)&Blobx2, FALSE);
    VariableDefinePlatformVar(NULL, "bloby1", &IntType, (union AnyValue *)&Bloby1, FALSE);
    VariableDefinePlatformVar(NULL, "bloby2", &IntType, (union AnyValue *)&Bloby2, FALSE);
    VariableDefinePlatformVar(NULL, "y1", &IntType, (union AnyValue *)&Iy1, FALSE);
    VariableDefinePlatformVar(NULL, "y2", &IntType, (union AnyValue *)&Iy2, FALSE);
    VariableDefinePlatformVar(NULL, "u1", &IntType, (union AnyValue *)&Iu1, FALSE);
    VariableDefinePlatformVar(NULL, "u2", &IntType, (union AnyValue *)&Iu2, FALSE);
    VariableDefinePlatformVar(NULL, "v1", &IntType, (union AnyValue *)&Iv1, FALSE);
    VariableDefinePlatformVar(NULL, "v2", &IntType, (union AnyValue *)&Iv2, FALSE);
    VariableDefinePlatformVar(NULL, "gpslatdeg", &IntType, (union AnyValue *)&GPSlatdeg, FALSE);
    VariableDefinePlatformVar(NULL, "gpslatmin", &IntType, (union AnyValue *)&GPSlatmin, FALSE);
    VariableDefinePlatformVar(NULL, "gpslondeg", &IntType, (union AnyValue *)&GPSlondeg, FALSE);
    VariableDefinePlatformVar(NULL, "gpslonmin", &IntType, (union AnyValue *)&GPSlonmin, FALSE);
    VariableDefinePlatformVar(NULL, "gpsalt", &IntType, (union AnyValue *)&GPSalt, FALSE);
    VariableDefinePlatformVar(NULL, "gpsfix", &IntType, (union AnyValue *)&GPSfix, FALSE);
    VariableDefinePlatformVar(NULL, "gpssat", &IntType, (union AnyValue *)&GPSsat, FALSE);
    VariableDefinePlatformVar(NULL, "gpsutc", &IntType, (union AnyValue *)&GPSutc, FALSE);
}

void Csignal(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  // check for kbhit, return t or nil
{
    if (getsignal())
        ReturnValue->Val->Integer = 1;
    else
        ReturnValue->Val->Integer = 0;
}

void Cinput(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  // return 0-9 from console input
{
    ReturnValue->Val->Integer = getch();
}

void Cdelay(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int del;
    
    del = Param[0]->Val->Integer;
    if ((del < 0) || (del > 1000000))
        return;
    delayMS(del);
}

void Crand(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = (int)rand() % (unsigned int)(Param[0]->Val->Integer + 1);
}

void Ctime(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = (int)readRTC();
}

void Ciodir(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int dir;
    
    dir = Param[0]->Val->Integer;
    *pPORTHIO_DIR = ((dir << 10) & 0xFC00) + (*pPORTHIO_DIR & 0x03FF);  // H15/14/13/12/11/10 - 1=output, 0=input
    *pPORTHIO_INEN = (((~dir) << 10) & 0xFC00) + (*pPORTHIO_INEN & 0x03FF); // invert dir bits to enable inputs
}

void Cioread(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = (*pPORTHIO >> 10) & 0x003F;
}

void Ciowrite(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    *pPORTHIO = ((Param[0]->Val->Integer << 10) & 0xFC00) + (*pPORTHIO & 0x03FF);
}

void Cpeek(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int size, ptr;
    unsigned char *cp;
    unsigned short *sp;
    unsigned int *ip;
    
    /* x = peek(addr, size);
       mask ptr to align with word size */
    ptr = Param[0]->Val->Integer;
    size = Param[1]->Val->Integer;
    switch (size) {
        case 1: // char *
            cp = (unsigned char *)ptr;
            ReturnValue->Val->Integer = (int)((unsigned int)*cp);
            break;
        case 2: // short *
            sp = (unsigned short *)(ptr & 0xFFFFFFFE);  // align with even boundary
            ReturnValue->Val->Integer = (int)((unsigned short)*sp);
            break;
        case 4: // int *
            ip = (unsigned int *)(ptr & 0xFFFFFFFC);  // aling with quad boundary
            ReturnValue->Val->Integer = (int)*ip;
            break;
        default:
            ReturnValue->Val->Integer = 0;
            break;
    }
}

void Cpoke(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int size, ptr, val;
    unsigned char *cp;
    unsigned short *sp;
    unsigned int *ip;
    
    /* x = poke(addr, size, val);
       mask ptr to align with word size */
    ptr = Param[0]->Val->Integer;
    size = Param[1]->Val->Integer;
    val = Param[2]->Val->Integer;
    switch (size) {
        case 1: // char *
            cp = (unsigned char *)ptr;
            *cp = (unsigned char)(val & 0x000000FF);
            break;
        case 2: // short *
            sp = (unsigned short *)(ptr & 0xFFFFFFFE);
            *sp = (unsigned short)(val & 0x0000FFFF);
            break;
        case 4: // int *
            ip = (unsigned int *)(ptr & 0xFFFFFFFC);
            *ip = val;
            break;
        default: // don't bother with bad value
            break;
    }
}

void Cmotors(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    lspeed = Param[0]->Val->Integer;
    if ((lspeed < -100) || (lspeed > 100))
        ProgramFail(NULL, "motors():  left motor value out of range");
    rspeed = Param[1]->Val->Integer;
    if ((rspeed < -100) || (rspeed > 100))
        ProgramFail(NULL, "motors():  right motor value out of range");
    if (!pwm1_init) {
        initPWM();
        pwm1_init = 1;
        pwm1_mode = PWM_PWM;
        base_speed = 50;
    }
    setPWM(lspeed, rspeed);
}

void Cmotors2(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    lspeed2 = Param[0]->Val->Integer;
    if ((lspeed2 < -100) || (lspeed2 > 100))
        ProgramFail(NULL, "motors2():  left motor value out of range");
    rspeed2 = Param[1]->Val->Integer;
    if ((rspeed2 < -100) || (rspeed2 > 100))
        ProgramFail(NULL, "motors2():  right motor value out of range");
    if (!pwm2_init) {
        initPWM2();
        pwm2_init = 1;
        pwm2_mode = PWM_PWM;
        base_speed2 = 50;
    }
    setPWM2(lspeed2, rspeed2);
}

void Cservos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int lspeed, rspeed;
    
    lspeed = Param[0]->Val->Integer;
    if ((lspeed < 0) || (lspeed > 100))
        ProgramFail(NULL, "servos():  TMR2 value out of range");
    rspeed = Param[1]->Val->Integer;
    if ((rspeed < 0) || (rspeed > 100))
        ProgramFail(NULL, "servos()():  TMR3 value out of range");
    if (!pwm1_init) {
        initPPM1();
        pwm1_init = 1;
        pwm1_mode = PWM_PPM;
    }
    setPPM1(lspeed, rspeed);
}

void Cservos2(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int lspeed, rspeed;
    
    lspeed = Param[0]->Val->Integer;
    if ((lspeed < 0) || (lspeed > 100))
        ProgramFail(NULL, "servos2():  TMR6 value out of range");
    rspeed = Param[1]->Val->Integer;
    if ((rspeed < 0) || (rspeed > 100))
        ProgramFail(NULL, "servos2():  TMR7 value out of range");
    if (!pwm2_init) {
        initPPM2();
        pwm2_init = 1;
        pwm2_mode = PWM_PPM;
    }
    setPPM2(lspeed, rspeed);
}

void Claser(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)    // laser(1) turns them on, laser(0) turns them off
{
    *pPORTHIO &= 0xFD7F;  // turn off both lasers
    switch (Param[0]->Val->Integer) {
        case 1:
            *pPORTHIO |= 0x0080;  // turn on left laser
            break;
        case 2:
            *pPORTHIO |= 0x0200;  // turn on right laser
            break;
        case 3:
            *pPORTHIO |= 0x0280;  // turn on both lasers
            break;
    }
}

void Csonar(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) // read sonar module
{
    unsigned int i;
    i = Param[0]->Val->Integer;
    if ((i<1) || (i>4)) {
        ProgramFail(NULL, "sonar():  1, 2, 3, 4 are only valid selections");
    }
    sonar();
    ReturnValue->Val->Integer = sonar_data[i] / 100;
}

void Crange(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = laser_range(0);
}

void Cbattery(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (*pPORTHIO & 0x0004)
        ReturnValue->Val->Integer = 0;  // low battery voltage detected
    else
        ReturnValue->Val->Integer = 1;  // battery voltage okay
}

void Cvcolor(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) // set color bin - 
                //    vcolor (color, ymin, ymax, umin, umax, vmin, vmax);                  
{
    int ix;
    
    ix = Param[0]->Val->Integer;
    ymin[ix] = Param[1]->Val->Integer;
    ymax[ix] = Param[2]->Val->Integer;
    umin[ix] = Param[3]->Val->Integer;
    umax[ix] = Param[4]->Val->Integer;
    vmin[ix] = Param[5]->Val->Integer;
    vmax[ix] = Param[6]->Val->Integer;
}

void Cvcam(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) // set camera functions - 
     //    enable/disable AGC(4) / AWB(2) / AEC(1) camera controls
     //    vcam(7) = AGC+AWB+AEC on   vcam(0) = AGC+AWB+AEC off
{
    unsigned char cx, i2c_data[2];

    cx = (unsigned char)Param[0]->Val->Integer & 0x07;
    i2c_data[0] = 0x13;
    i2c_data[1] = 0xC0 + cx;
    i2cwrite(0x30, (unsigned char *)i2c_data, 1, SCCB_ON);  // OV9655
    i2cwrite(0x21, (unsigned char *)i2c_data, 1, SCCB_ON);  // OV7725
}

void Cvfind(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) // set color bin - 
                //    vfind (color, x1, x2, y1, y2);                  
{
    int ix, x1, x2, y1, y2;
    
    ix = Param[0]->Val->Integer;
    x1 = Param[1]->Val->Integer;
    x2 = Param[2]->Val->Integer;
    y1 = Param[3]->Val->Integer;
    y2 = Param[4]->Val->Integer;
    ReturnValue->Val->Integer = vfind((unsigned char *)FRAME_BUF, ix, x1, x2, y1, y2);
}

void Cvcap(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    grab_frame();   // capture frame for processing
}

void Cvrcap(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    grab_reference_frame();   // capture reference frame for differencing
}

void Cvdiff(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    frame_diff_flag = Param[0]->Val->Integer;   // set/clear frame_diff_flag
}

void Cvpix(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int x, y, ix;
    x = Param[0]->Val->Integer;
    y = Param[1]->Val->Integer;
    ix = vpix((unsigned char *)FRAME_BUF, x, y);
    Iy1 = ((ix>>16) & 0x000000FF);  // Y1
    Iu1 = ((ix>>24) & 0x000000FF);  // U
    Iv1 = ((ix>>8)  & 0x000000FF);  // V
}

void Cvscan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int col, thresh, ix;
    col = Param[0]->Val->Integer;
    if ((col < 1) || (col > 9))
        ProgramFail(NULL, "vscan():  number of columns must be between 1 and 9");
    thresh = Param[1]->Val->Integer;
    if ((thresh < 0) || (thresh > 9999)) 
        ProgramFail(NULL, "vscan():  threshold must be between 0 and 9999");
    ix = vscan((unsigned char *)SPI_BUFFER1, (unsigned char *)FRAME_BUF, thresh, (unsigned int)col, (unsigned int *)&ScanVect[0]);
    ReturnValue->Val->Integer = ix;
}

void Cvmean(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) 
{
    vmean((unsigned char *)FRAME_BUF);
    Iy1 = mean[0];
    Iu1 = mean[1];
    Iv1 = mean[2];
}

//    search for blob by color, index;  return center point X,Y and width Z
void Cvblob(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)    {
    int ix, iblob, numblob;

    ix = Param[0]->Val->Integer;
    if (ix > MAX_COLORS)
        ProgramFail(NULL, "blob():  invalid color index");
    iblob = Param[1]->Val->Integer;
    if (iblob > MAX_BLOBS)
        ProgramFail(NULL, "blob():  invalid blob index");
        
    numblob = vblob((unsigned char *)FRAME_BUF, (unsigned char *)FRAME_BUF3, ix);

    if (blobcnt[iblob] == 0) {
        Blobcnt = 0;
    } else {
        Blobcnt = blobcnt[iblob];
        Blobx1 = blobx1[iblob];
        Blobx2 = blobx2[iblob];
        Bloby1 = bloby1[iblob];
        Bloby2 = bloby2[iblob];
    }
    ReturnValue->Val->Integer = numblob;
}

void Ccompass(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  // return reading from HMC6352 I2C compass
{
    unsigned char i2c_data[2];
    unsigned int ix;
    
    i2c_data[0] = 0x41;  // read compass twice to clear last reading
    i2cread(0x22, (unsigned char *)i2c_data, 2, SCCB_ON);
    delayMS(10);
    i2c_data[0] = 0x41;
    i2cread(0x22, (unsigned char *)i2c_data, 2, SCCB_ON);
    ix = ((unsigned int)(i2c_data[0] << 8) + i2c_data[1]) / 10;
    ReturnValue->Val->Integer = ix;
}

void Ctilt(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  // return reading from HMC6352 I2C compass
{
    unsigned int ix;
    
    ix = (unsigned int)Param[0]->Val->Integer;
    if ((ix<1) || (ix>3))
        ProgramFail(NULL, "tilt():  invalid channel");
    ReturnValue->Val->Integer = tilt(ix);
}

void Canalog(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  // return reading from HMC6352 I2C compass
{
    unsigned char i2c_data[3], device_id;
    unsigned int ix, channel;
    unsigned char mask1[] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x08 };
    unsigned char mask2[] = { 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00 };
    
    // decide which i2c device based on channel range
    ix = (unsigned char)Param[0]->Val->Integer;
    if ((ix<1) || (ix>28))
        ProgramFail(NULL, "analog():  invalid channel");
    device_id = 0;
    switch (ix / 10) {
        case 0:
            device_id = 0x20;  // channels 1-8
            break;
        case 1:
            device_id = 0x23;  // channels 11-18
            break;
        case 2:
            device_id = 0x24;  // channels 21-28
            break;
    }
    channel = ix % 10;
    if ((channel<1) || (channel>8))
        ProgramFail(NULL, "analog():  invalid channel");
    
    // set timer register 3
    i2c_data[0] = 0x03;
    i2c_data[1] = 0x01;
    i2cwrite(device_id, (unsigned char *)i2c_data, 1, SCCB_ON);

    // set analog channel 
    i2c_data[0] = 0x02;
    i2c_data[1] = mask1[channel-1];
    i2c_data[2] = mask2[channel-1];
    i2cwritex(device_id, (unsigned char *)i2c_data, 3, SCCB_ON);

    // small delay
    delayUS(1000);

    // read data
    i2c_data[0] = 0x00;
    i2cread(device_id, (unsigned char *)i2c_data, 2, SCCB_ON);
    ix = (((i2c_data[0] & 0x0F) << 8) + i2c_data[1]);
    ReturnValue->Val->Integer = ix;
}

void Cgps(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    gps_parse();
    GPSlatdeg = gps_gga.latdeg;
    GPSlatmin = gps_gga.latmin;
    GPSlondeg = gps_gga.londeg;
    GPSlonmin = gps_gga.lonmin;
    GPSalt = gps_gga.alt;
    GPSfix = gps_gga.fix;
    GPSsat = gps_gga.sat;
    GPSutc = gps_gga.utc;
}

void Creadi2c(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  //  syntax   val = readi2c(device, register);
{
    unsigned char i2c_device, i2c_data[2];
    
    i2c_device = (unsigned char)Param[0]->Val->Integer;
    i2c_data[0] = (unsigned char)Param[1]->Val->Integer;
    
    i2cread(i2c_device, (unsigned char *)i2c_data, 1, SCCB_OFF);
    ReturnValue->Val->Integer = ((int)i2c_data[0] & 0x000000FF);
}

void Creadi2c2(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  //  syntax   two_byte_val = readi2c(device, register); 
{
    unsigned char i2c_device, i2c_data[2];

    i2c_device = (unsigned char)Param[0]->Val->Integer;
    i2c_data[0] = (unsigned char)Param[1]->Val->Integer;
    
    i2cread(i2c_device, (unsigned char *)i2c_data, 2, SCCB_OFF);
    ReturnValue->Val->Integer = (((unsigned int)i2c_data[0] << 8) + i2c_data[1]);
}

void Cwritei2c(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  //  syntax   writei2c(device, register, value);
{
    unsigned char i2c_device, i2c_data[2];

    i2c_device = (unsigned char)Param[0]->Val->Integer;
    i2c_data[0] = (unsigned char)Param[1]->Val->Integer;
    i2c_data[1] = (unsigned char)Param[2]->Val->Integer;
    
    i2cwrite(i2c_device, (unsigned char *)i2c_data, 1, SCCB_OFF);
}

static int cosine[] = {
10000, 9998, 9994, 9986, 9976, 9962, 9945, 9925, 9903, 9877, 
 9848, 9816, 9781, 9744, 9703, 9659, 9613, 9563, 9511, 9455, 
 9397, 9336, 9272, 9205, 9135, 9063, 8988, 8910, 8829, 8746, 
 8660, 8572, 8480, 8387, 8290, 8192, 8090, 7986, 7880, 7771, 
 7660, 7547, 7431, 7314, 7193, 7071, 6947, 6820, 6691, 6561, 
 6428, 6293, 6157, 6018, 5878, 5736, 5592, 5446, 5299, 5150, 
 5000, 4848, 4695, 4540, 4384, 4226, 4067, 3907, 3746, 3584, 
 3420, 3256, 3090, 2924, 2756, 2588, 2419, 2250, 2079, 1908, 
 1736, 1564, 1392, 1219, 1045,  872,  698,  523,  349,  175, 
    0 
};

void Csin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  // sin(angle)
{
    int ix;
    
    ix = Param[0]->Val->Integer;  // input to function is angle in degrees
    while (ix < 0)
        ix = ix + 360;
    while (ix >= 360)
        ix = ix - 360;
    if (ix < 90)  { ReturnValue->Val->Integer = cosine[90-ix] / 100;  return; }
    if (ix < 180) { ReturnValue->Val->Integer = cosine[ix-90] / 100;  return; }
    if (ix < 270) { ReturnValue->Val->Integer = -cosine[270-ix] / 100;  return; }
    if (ix < 360) { ReturnValue->Val->Integer = -cosine[ix-270] / 100;  return; }
}

void Ccos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  // cos(angle)
{
    int ix;
    
    ix = Param[0]->Val->Integer;  // input to function is angle in degrees
    while (ix < 0)
        ix = ix + 360;
    while (ix >= 360)
        ix = ix - 360;
    if (ix < 90)  { ReturnValue->Val->Integer = cosine[ix] / 100;  return; }
    if (ix < 180) { ReturnValue->Val->Integer = -cosine[180-ix] / 100;  return; }
    if (ix < 270) { ReturnValue->Val->Integer = -cosine[ix-180] / 100;  return; }
    if (ix < 360) { ReturnValue->Val->Integer = cosine[360-ix] / 100;  return; }
}

void Ctan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  // tan(angle)
{
    int ix;
    
    ix = Param[0]->Val->Integer;  // input to function is angle in degrees
    while (ix < 0)
        ix = ix + 360;
    while (ix >= 360)
        ix = ix - 360;
    if (ix == 90)  { ReturnValue->Val->Integer = 9999;  return; }
    if (ix == 270) { ReturnValue->Val->Integer = -9999;  return; }
    if (ix < 90)   { ReturnValue->Val->Integer = (100 * cosine[90-ix]) / cosine[ix];  return; }
    if (ix < 180)  { ReturnValue->Val->Integer = -(100 * cosine[ix-90]) / cosine[180-ix];  return; }
    if (ix < 270)  { ReturnValue->Val->Integer = (100 * cosine[270-ix]) / cosine[ix-180];  return; }
    if (ix < 360)  { ReturnValue->Val->Integer = -(100 * cosine[ix-270]) / cosine[360-ix];  return; }
}

void Casin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  // asin(y,hyp)
{
    int y, hyp, quot, sgn, ix;
    y = Param[0]->Val->Integer;
    hyp = Param[1]->Val->Integer;
    if (y > hyp)
        ProgramFail(NULL, "asin():  opposite greater than hypotenuse");
    if (y == 0) {
        ReturnValue->Val->Integer = 0;
        return;
    }
    sgn = hyp * y;
    if (hyp < 0) 
        hyp = -hyp;
    if (y < 0)
        y = -y;
    quot = (y * 10000) / hyp;
    if (quot > 9999)
        quot = 9999;
    for (ix=0; ix<90; ix++)
        if ((quot < cosine[ix]) && (quot >= cosine[ix+1]))
            break;
    if (sgn < 0)
        ReturnValue->Val->Integer = -(90-ix);
    else
        ReturnValue->Val->Integer = 90-ix;
}

void Cacos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  // acos(x,hyp)
{
    int x, hyp, quot, sgn, ix;
    x = Param[0]->Val->Integer;
    hyp = Param[1]->Val->Integer;
    if (x > hyp)
        ProgramFail(NULL, "acos():  adjacent greater than hypotenuse");
    if (x == 0) {
        if (hyp < 0)
            ReturnValue->Val->Integer = -90;
        else
            ReturnValue->Val->Integer = 90;
        return;
    }
    sgn = hyp * x;
    if (hyp < 0) 
        hyp = -hyp;
    if (x < 0)
        x = -x;
    quot = (x * 10000) / hyp;
    if (quot > 9999)
        quot = 9999;
    for (ix=0; ix<90; ix++)
        if ((quot < cosine[ix]) && (quot >= cosine[ix+1]))
            break;
    if (sgn < 0)
        ReturnValue->Val->Integer = -ix;
    else
        ReturnValue->Val->Integer = ix;
}

void Catan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)  // atan(y,x)
{
    int x,y, angle, coeff_1, coeff_2, r;
    y = Param[0]->Val->Integer;
    x = Param[1]->Val->Integer;
    if (x == 0) {
        if (y >= 0)
            ReturnValue->Val->Integer = 90;
        else
            ReturnValue->Val->Integer = -90;
        return;
    }
    coeff_1 = 3141/4;
    coeff_2 = coeff_1*3;
    if (y < 0)
        y = -y;
    if (x >= 0) {
        r = (x - y)*1000 / (x + y);
        angle = (coeff_1*1000 - coeff_1 * r);
    } else {
        r = (x + y)*1000 / (y - x);
        angle = (coeff_2*1000 - coeff_1 * r);
    }
    angle = angle*57/1000000;
    if (y < 0)
        ReturnValue->Val->Integer = -angle;
    else
        ReturnValue->Val->Integer = angle;
} 

void Cnnset(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) {
    int ix, i1;
    
    ix = Param[0]->Val->Integer;
    if (ix > NUM_NPATTERNS)
        ProgramFail(NULL, "nnset():  invalid index");
    for (i1=0; i1<8; i1++)
        npattern[ix*8 + i1] = (unsigned char)Param[i1+1]->Val->Integer;
}

void Cnnshow(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) {
    int ix;
    
    ix = Param[0]->Val->Integer;
    if (ix > NUM_NPATTERNS)
        ProgramFail(NULL, "nnshow():  invalid index");
    nndisplay(ix);
}

void Cnninit(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) {
    nninit_network();
}

void Cnntrain(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) {
    int ix, i1;

    nntrain_network(10000);
    for (ix=0; ix<NUM_NPATTERNS; ix++) {
        nnset_pattern(ix);
        nncalculate_network();
        for (i1=0; i1<NUM_OUTPUT; i1++) 
            printf(" %3d", N_OUT(i1)/10);
        printf("\n\r");
    }
}

void Cnntest(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) {
    int ix, i1, i2;
    unsigned char ch;
    
    ix = 0;
    for (i1=0; i1<8; i1++) {
        ch = (unsigned char)Param[i1]->Val->Integer;
        for (i2=0; i2<8; i2++) {
            if (ch & nmask[i2])
                N_IN(ix++) = 1024;
            else
                N_IN(ix++) = 0;
        }
    }
    nncalculate_network();
    for (i1=0; i1<NUM_OUTPUT; i1++) 
        printf(" %3d", N_OUT(i1)/10);
    printf("\n\r");
}

void Cnnmatchblob(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) {            
    int ix, i1;
    
    ix = Param[0]->Val->Integer;
    if (ix > MAX_BLOBS)
        ProgramFail(NULL, "nnmatchblob():  invalid blob index");
    if (!blobcnt[ix])
        ProgramFail(NULL, "nnmatchblob():  not a valid blob");
    /* use data still in blob_buf[] (FRAME_BUF3)
       square the aspect ratio of x1, x2, y1, y2
       then subsample blob pixels to populate N_IN(0:63) with 0:1024 values
       then nncalculate_network() and display the N_OUT() results */
    nnscale8x8((unsigned char *)FRAME_BUF3, blobix[ix], blobx1[ix], blobx2[ix], 
            bloby1[ix], bloby2[ix], imgWidth, imgHeight);
    nncalculate_network();
    for (i1=0; i1<NUM_OUTPUT; i1++) 
        printf(" %3d", N_OUT(i1)/10);
    printf("\n\r");
}

void Cnnlearnblob (struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) {
    int ix;
    
    ix = Param[0]->Val->Integer;
    if (ix > NUM_NPATTERNS)
        ProgramFail(NULL, "nnlearnblob():  invalid index");
    if (!blobcnt[0])
        ProgramFail(NULL, "nnlearnblob():  no blob to grab");
    nnscale8x8((unsigned char *)FRAME_BUF3, blobix[0], blobx1[0], blobx2[0], 
            bloby1[0], bloby2[0], imgWidth, imgHeight);
    nnpack8x8(ix);
    nndisplay(ix);
}

void Cexit (struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) {
    ExitBuf[40] = 1;
    longjmp(ExitBuf, 1);
}

/* list of all library functions and their prototypes */
struct LibraryFunction PlatformLibrary[] =
{
    { Csignal,      "int signal()" },
    { Cinput,       "int input()" },
    { Cdelay,       "void delay(int)" },
    { Crand,        "int rand(int)" },
    { Ctime,        "int time()" },
    { Ciodir,       "void iodir(int)" },
    { Cioread,      "int ioread()" },
    { Ciowrite,     "void iowrite(int)" },
    { Cpeek,        "int peek(int, int)" },
    { Cpoke,        "void poke(int, int, int)" },
    { Cmotors,      "void motors(int, int)" },
    { Cmotors2,     "void motors2(int, int)" },
    { Cservos,      "void servos(int, int)" },
    { Cservos2,     "void servos2(int, int)" },
    { Claser,       "void laser(int)" },
    { Csonar,       "int sonar(int)" },
    { Crange,       "int range()" },
    { Cbattery,     "int battery()" },
    { Cvcolor,      "void vcolor(int, int, int, int, int, int, int)" },
    { Cvfind,       "int vfind(int, int, int, int, int)" },
    { Cvcam,        "void vcam(int)" },
    { Cvcap,        "void vcap()" },
    { Cvrcap,       "void vrcap()" },
    { Cvdiff,       "void vdiff(int)" },
    { Cvpix,        "void vpix(int, int)" },
    { Cvscan,       "int vscan(int, int)" },
    { Cvmean,       "void vmean()" },
    { Cvblob,       "int vblob(int, int)" },
    { Ccompass,     "int compass()" },
    { Canalog,      "int analog(int)" },
    { Ctilt,        "int tilt(int)" },
    { Cgps,         "void gps()" },
    { Creadi2c,     "int readi2c(int, int)" },
    { Creadi2c2,    "int readi2c2(int, int)" },
    { Cwritei2c,    "void writei2c(int, int, int)" },
    { Csin,         "int sin(int)" },
    { Ccos,         "int cos(int)" },
    { Ctan,         "int tan(int)" },
    { Casin,        "int asin(int, int)" },
    { Cacos,        "int acos(int, int)" },
    { Catan,        "int atan(int, int)" },
    { Cnnshow,      "void nnshow(int)" },
    { Cnnset,       "void nnset(int, int, int, int, int, int, int, int, int)" },
    { Cnninit,      "void nninit()" },
    { Cnntrain,     "void nntrain()" },
    { Cnntest,      "void nntest(int, int, int, int, int, int, int, int)" },
    { Cnnmatchblob, "void nnmatchblob(int)" },
    { Cnnlearnblob, "void nnlearnblob(int)" },
    { Cexit,        "void exit()" },
    { NULL,         NULL }
};

