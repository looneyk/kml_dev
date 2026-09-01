
{
    double samples[1024];            /* input data, 1024 samples*/
    double *fft_out ;              /* output*/
    fftw_complex *out_cpx;         /* fft result */
    fftw_complex *out_cpx1;        /* this array will contain new, modified complex numbers*/
    fftw_plan fft;
    fftw_plan ifft;
    out_cpx = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * 513);
    out_cpx1 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * 513);
    fft_out = (double *) malloc(1024*sizeof(double));
    fft = fftw_plan_dft_r2c_1d(1024, samples, out_cpx, FFTW_ESTIMATE);
    ifft = fftw_plan_dft_c2r_1d(1024, out_cpx1, fft_out, FFTW_ESTIMATE);

    fftw_execute(fft);

    for ( i=0; i<513; i++)     /* now looping through fft complex array*/
    {
        double complex z1 = out_cpx[i][0] + out_cpx[i][1] * I;
        double amp = cabs(z1) ;
        double ph = phase(creal(z1),cimag(z1));     /* phase angle*/
        double new_amplitude;

        if (341<i<513)                /* selected region; 1/3 of array selected to be modified, the third with the highest freqs*/
        {
            new_amplitude = amp * ?    **/* this is where it struggles; which ever way I apply the boost, it doesn't work. */**
        }
        else
        {
            new_amplitude = amp;         /* rest of frequencies; no scaling
        }
        /* new complex numbers */
        double re = new_amplitude * cos(ph);    /* real part*/
        double im = new_amplitude * sin(ph);    /* imaginary */
        out_cpx1[i][0] = re;
        out_cpx1[i][1] = im;
    }

    fftw_execute(ifft);

    for ( i=0; i<1023; i++)
    {
        fft_out[i] = fft_out[i]/1024.0;
    }
}

double phase (double x, double y)
{
    double ph;
    if (x > 0)
        ph = atan(y/x);

    if (x < 0)
    {
        if (y >= 0)
            ph = atan(y/x) + M_PI;
        if (y < 0)
            ph = atan(y/x) - M_PI;
    }
}
