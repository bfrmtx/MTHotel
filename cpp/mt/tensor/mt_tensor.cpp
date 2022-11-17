#include "mt_tensor.h"

mt_tensor::mt_tensor()
{

}

void mt_tensor::clear()
{
    this->f = 0.0;
    this->selected = true;

    this->xx = std::complex<double>(0.0);
    this->xy = std::complex<double>(0.0);
    this->yx = std::complex<double>(0.0);
    this->yy = std::complex<double>(0.0);

    this->tx = std::complex<double>(0.0);
    this->ty = std::complex<double>(0.0);

    this->d_xx = 0.0;
    this->d_xy = 0.0;
    this->d_yx = 0.0;
    this->d_yy = 0.0;

    this->d_tx = 0.0;
    this->d_ty = 0.0;


    this->alpha = 0.0;
    this->d_alpha = 0.0;

    this->avgt = 0.0;
    this->avgf = 0.0;
    this->bw = 0.0;
}
