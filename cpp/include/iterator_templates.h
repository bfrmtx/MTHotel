#ifndef ITERATOR_TEMPLATES
#define ITERATOR_TEMPLATES
#include <cmath>
namespace miter {

/*!
 * \brief cleanup set elements to zero wich absloute value is smaller than treat_as_zero
 * \param first double iter
 * \param last double iter
 * \param treat_as_zero something like 1E-32
 */
template<typename Iterator>
void cleanup(Iterator first, Iterator last, const double treat_as_zero) {

    while (first != last) {
        if (std::fabs(*first) < treat_as_zero) *first = 0.0;
        ++first;
    }
}


/*!
 * \brief cleanup_cplx set elements to zero (real AND/OR imaginary) wich absloute value is smaller than treat_as_zero
 * \param first complex double iter
 * \param last complex double iter
 * \param treat_as_zero something like 1E-32
 */
template<typename Iterator>
void cleanup_cplx(Iterator first, Iterator last, const double treat_as_zero) {

    while (first != last) {
        if (std::fabs(real(*first)) < treat_as_zero) *first = complex<double>(0.0, imag(*first));
        if (std::fabs(imag(*first)) < treat_as_zero) *first = complex<double>(real(*first), 0.0);

        ++first;
    }
}

} // end namespace


#endif
