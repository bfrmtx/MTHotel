


#include <QCoreApplication>
#include <complex>
#include <iostream>
#include <valarray>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTime>
#include <QDebug>
#include <functional>
#include "FFTReal.h"
#include "FFTRealFixLen.h"

#include <fftw3.h>


using namespace std;
//using namespace ffft;



//const double PI = 3.141592653589793238460;

//typedef std::complex<double> Complex;
//typedef std::valarray<Complex> CArray;

//// Cooley–Tukey FFT (in-place)
//void fft(std::valarray<complex<double> >& x)
//{
//    const size_t N = x.size();
//    if (N <= 1) return;

//    // divide
//    std::valarray<complex<double> > even = x[std::slice(0, N/2, 2)];
//    std::valarray<complex<double> >  odd = x[std::slice(1, N/2, 2)];

//    // conquer
//    fft(even);
//    fft(odd);

//    // combine
//    for (size_t k = 0; k < N/2; ++k)
//    {
//        std::complex<double> t = std::polar(1.0, -2 * M_PI * k / N) * odd[k];
//        x[k    ] = even[k] + t;
//        x[k+N/2] = even[k] - t;
//    }
//}

// inverse fft (in-place)
//void ifft(CArray& x)
//{
//    // conjugate the complex numbers
//    x = x.apply(std::conj);

//    // forward fft
//    fft( x );

//    // conjugate the complex numbers again
//    x = x.apply(std::conj);

//    // scale the numbers
//    x /= x.size();
//}

//class vfft
//{
//public:
//    vfft() {}
//    virtual void do_fft (double* in, const double* out) = 0;
//};

//class vfft4096 : public vfft
//{
//public:
//    vfft4096() {}
//    void do_fft (double* in, const double* out) {fft_object.do_fft(in, out);}
//private:
//    ffft::FFTRealFixLen <12> fft_object;
//};

//class vfft1024 : public vfft
//{
//public:
//    vfft1024() {}
//    void do_fft (double* in, const double* out) {fft_object.do_fft(in, out);}

//private:
//    ffft::FFTRealFixLen <10> fft_object;
//};



int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    int i, j;
    int N = 8;

    int ntests = 1;

    ffft::FFTReal <double> fft_object (N);

    ffft::FFTRealFixLen <12> *fft_object_fix_4096 = new(ffft::FFTRealFixLen <12>);
    ffft::FFTRealFixLen <10> *fft_object_fix_1024 = new(ffft::FFTRealFixLen <10>);





    valarray<double>  in(N);
    valarray<complex<double> > out(N);

    fftw_plan p;
    double *fftwin = &in[0];
    valarray<complex<double> > vfftwout(N);
    complex<double> *fftwout = &vfftwout[0];


    QTime tfft;
    QTime tfftw;
    QTime tfftreal;
    QTime tfftreal_fix;


    //    const Complex test[] = { 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0 };
    //       CArray data(test, 8);

    QFile qfi_in("/home/bfr/myfft/084_V01_C02_R001_THx_BL_2S.dat");
    QFile qfi_out("/home/bfr/build-myfft-Desktop-Debug/ampl_1.dat");
    QFile qfi_outfftw("/home/bfr/build-myfft-Desktop-Debug/ampl_fftw.dat");

    QTextStream qts(&qfi_in);

//    qfi_in.open(QFile::ReadOnly | QFile::Text);

//    for (i = 0; i < N; ++i) {
//        qts >> in[i];
//    }
//    qfi_in.close();




    int r = 2;

    for ( i = 0; i < r; ++i) {
        for (j = 0; j < N; ++j) {
         if (i == 0) in[j] = 6*i*r+j*j;
        }
    }


    //CArray data(N);

    //    for  (i = 0; i < N/2; ++i ) in[i] = 1;
    //    for  (i = N/2; i < N; ++i ) in[i] = 0;

    // for (i = 0; i < in.size(); ++i) out[i] = in[i];

    //    std::cout << "input" << std::endl;
    //    for (int i = 1; i < out.size()/2; ++i)
    //    {
    //        std::cout << abs(out[i]) << std::endl;
    //    }


    // forward fft
    //    tfft.start();

    //    for (i = 0; i < ntests; ++i) {
    //        for (j = 0; j < in.size(); ++j) out[j] = in[j];

    //        fft(out);
    //    }

    //    qDebug() << "FFT" << tfft.elapsed();

    // std::cout << "fft" << std::endl;
    //    for (int i = 1; i < out.size()/2; ++i)
    //    {
    //        std::cout << abs(out[i]) << std::endl;
    //    }




    cout << endl << endl;


    // check the FFTW speed

    tfftw.start();

    for (i = 0; i < ntests; ++i) {

        p = fftw_plan_dft_r2c_1d(N, fftwin, reinterpret_cast<fftw_complex*>(fftwout) , FFTW_ESTIMATE);
        fftw_execute(p);
        fftw_destroy_plan(p);

    }

    qDebug() << "FFTW" << tfftw.elapsed();


    valarray<double> cv(N), bv(N);
    valarray<complex<double> > couts(N/2), coutsfix(N/2);



    // check the variable length FFTReal

    tfftreal.start();
    for (i = 0; i < ntests; ++i) {

        fft_object.do_fft (&bv[0], &in[0]);     // x (real) --FFT---> f (complex)
        for (j = 0; j < N/2; ++j) couts[j] = complex<double> (bv[j], bv[N/2 + j]);
    }

    qDebug() << "FFTREAL" << tfftreal.elapsed();



//    // check the fix length FFTReal

//    tfftreal_fix.start();
//    for (i = 0; i < ntests; ++i) {
//        fft_object_fix_4096->do_fft (&cv[0], &in[0]);     // x (real) --FFT---> f (complex)
//        //fixvar->do_fft (&cv[0], &in[0]);
//        for (j = 0; j < N/2; ++j) coutsfix[j] = complex<double> (cv[j], cv[N/2 + j]);
//    }

//    qDebug() << "FFTREAL FIX" << tfftreal_fix.elapsed();

    cout << endl;


 for (int i = 0; i < N; ++i) {

     std::cout << in[i] << endl;
 }
    for (int i = 0; i < N/2; ++i) {
        std::cout << vfftwout[i] << "  " << couts[i] << "  "  << std::endl;
    }


    return 0;
}
