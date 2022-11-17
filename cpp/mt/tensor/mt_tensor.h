#ifndef MT_TENSOR_H
#define MT_TENSOR_H

#include <complex>

class mt_tensor
{
public:
    mt_tensor();

    mt_tensor(const mt_tensor& rhs) {
        this->operator = (rhs);
    }

    void clear();

    mt_tensor operator = (const mt_tensor& rhs) {
        if (&rhs == this) return *this;
        this->f = rhs.f;
        this->selected = rhs.selected;
        this->f = rhs.f;
        this->xx = rhs.xx;
        this->xy = rhs.xy;
        this->yx = rhs.yx;
        this->yy = rhs.yy;


        this->tx = rhs.tx;
        this->ty = rhs.ty;

        this->d_xx = rhs.d_xx;
        this->d_xy = rhs.d_xy;
        this->d_yx = rhs.d_yx;
        this->d_yy = rhs.d_yy;


        this->d_tx = rhs.d_tx;
        this->d_ty = rhs.d_ty;

        this->alpha = rhs.alpha;
        this->d_alpha = rhs.d_alpha;

        this->avgt = rhs.avgt;
        this->avgf = rhs.avgf;
        this->bw = rhs.bw;


        return *this;
    }

    double f = 0.0;
    bool selected = true;

    std::complex<double> xx = std::complex<double>(0.0);
    std::complex<double> xy = std::complex<double>(0.0);
    std::complex<double> yx = std::complex<double>(0.0);
    std::complex<double> yy = std::complex<double>(0.0);

    std::complex<double> tx = std::complex<double>(0.0);
    std::complex<double> ty = std::complex<double>(0.0);

    double d_xx = 0.0;
    double d_xy = 0.0;
    double d_yx = 0.0;
    double d_yy = 0.0;

    double d_tx = 0.0;
    double d_ty = 0.0;


    double alpha = 0.0;
    double d_alpha = 0.0;

    double avgt = 0.0;                 //!< average in time domain
    double avgf = 0.0;                 //!< average in frequency domain, same as stacks but with weightning in case
    double bw = 0.0;                   //!< bandwidth

};

#endif // MT_TENSOR_H
