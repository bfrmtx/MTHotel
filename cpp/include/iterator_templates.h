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
template <typename Iterator>
void cleanup(Iterator first, Iterator last, const double &treat_as_zero) {

  while (first != last) {
    if (std::abs(*first) < treat_as_zero)
      *first = 0.0;
    ++first;
  }
}

/*!
 * \brief cleanup_cplx set elements to zero (real AND/OR imaginary) wich absloute value is smaller than treat_as_zero
 * \param first complex double iter
 * \param last complex double iter
 * \param treat_as_zero something like 1E-32
 */
template <typename Iterator>
void cleanup_cplx(Iterator first, Iterator last, const double &treat_as_zero) {

  while (first != last) {
    if (std::abs(real(*first)) < treat_as_zero)
      *first = complex<double>(0.0, imag(*first));
    if (std::abs(imag(*first)) < treat_as_zero)
      *first = complex<double>(real(*first), 0.0);

    ++first;
  }
}

//**************************************************  A K I M A   S P L I N E   *******************************************************

/*! \brief m little helper for Akima Splines ; dx(i) (x[i+1]-x[i]); dy(i) (y[i+1]-y[i]) and m = dy(i)/dx(i)
 * (random access seems not to want const qualifier)
 * @param x abscissa in
 * @param y ordinate in
 * @param index index on abscissa
 * @return dy[i] / dx[i]
 *
 */
template <class T, class const_iterator_x>
T m(const const_iterator_x x, const const_iterator_x y, const std::size_t index) {

  T dx = x[index + 1] - x[index];
  T dy = y[index + 1] - y[index];
  return dy / dx;
}

/*!
 * \brief aspline  Calculates the akima spline parameters b, c, d for  given inputs (x ,y) where x[0] < x[1] < ... < x[n] <br>
 * so the x-axis numbers MUST be SORTED x[0] < x[1] < x[n]<br>
 * RANDOM ACCESS Iterators needed - this template can only be used where "[]" is defined --- and not only "++"
 *
 * x = complete frequency list from cal file
 * y = complete spectra (real/imag) from cal file
 * b = 1st coefficient (output) computed from input
 * c = 2nd coefficient (output) computed from input
 * d = 3rd coefficient (output) computed from input
 *
 * for each input pair in x/y one set of coefffiecents b/c/d is computed.
 *
 * @param x_first first x Iterator
 * @param x_last  last x Iterator
 * @param y_first first y Iterator corresponding with first x!  -> (&x[0], &x[n+1])
 * @param y_last  last y Iterator corresponding with last x!  -> (&x[n], &y[n+1])
 * @param b_first coefficients start  - same size as 2D input (x,y)
 * @param b_last  coefficients stop
 * @param c_first coefficients start  - same size as 2D input (x,y)
 * @param c_last  coefficients stop
 * @param d_first coefficients start  - same size as 2D input (x,y)
 * @param d_last  coefficients stop
 * @return size of input
 */

template <class T, class const_iterator_x, class iterator_s>
std::size_t aspline(
    const const_iterator_x x_first, const const_iterator_x x_last,
    const const_iterator_x y_first, const const_iterator_x y_last,
    iterator_s b_first, iterator_s b_last,
    iterator_s c_first, iterator_s c_last,
    iterator_s d_first, iterator_s d_last) {

  long n = (std::size_t)(x_last - x_first);
  std::size_t i;

  // check for duplicates!!!
  //    for (i = 0; i < n - 1; i++)
  //      if (x[i] == x[i+1]) cerr << "aspline -> duplicate entry on x-axis( " << x[i] << ", " << x[i+1] << "); //result unstable; continued ****************" << endl;
  //

  if ((b_last - b_first) != n || (c_last - c_first) != n || (d_last - d_first) != n || (y_last - y_first) != n) {
#ifdef C_ERROR_OUT_MTX_ITER
    std::cerr << "aspline size mismatch " << std::endl;
#endif
    exit(0);
  }

  T num, den;
  T m_m1, m_m2, m_p1, m_p2;
  T x_m1, x_m2, x_p1, x_p2;
  T y_m1, y_m2, y_p1, y_p2;

  if (n > 0) {

    x_m1 = x_first[0] + x_first[1] - x_first[2];
    y_m1 = (x_first[0] - x_m1) * (m<T>(x_first, y_first, 1) - 2 * m<T>(x_first, y_first, 0)) + y_first[0];

    m_m1 = (y_first[0] - y_m1) / (x_first[0] - x_m1);

    x_m2 = 2 * x_first[0] - x_first[2];
    y_m2 = (x_m1 - x_m2) * (m<T>(x_first, y_first, 0) - 2 * m_m1) + y_m1;

    m_m2 = (y_m1 - y_m2) / (x_m1 - x_m2);

    x_p1 = x_first[n - 1] + x_first[n - 2] - x_first[n - 3];
    y_p1 = (2 * m<T>(x_first, y_first, (n - 2)) - m<T>(x_first, y_first, (n - 3))) * (x_p1 - x_first[n - 1]) + y_first[n - 1];

    m_p1 = (y_p1 - y_first[n - 1]) / (x_p1 - x_first[n - 1]);

    x_p2 = 2 * x_first[n - 1] - x_first[n - 3];
    y_p2 = (2 * m_p1 - m<T>(x_first, y_first, (n - 2))) * (x_p2 - x_p1) + y_p1;

    m_p2 = (y_p2 - y_p1) / (x_p2 - x_p1);

    // i = 0
    num = fabs(m<T>(x_first, y_first, 1) - m<T>(x_first, y_first, 0)) * m_m1 + fabs(m_m1 - m_m2) * m<T>(x_first, y_first, 0);
    den = fabs(m<T>(x_first, y_first, 1) - m<T>(x_first, y_first, 0)) + fabs(m_m1 - m_m2);

    if (den != 0.0)
      b_first[0] = num / den;
    else
      b_first[0] = 0.0;

    // i = 1
    num = fabs(m<T>(x_first, y_first, 2) - m<T>(x_first, y_first, 1)) * m<T>(x_first, y_first, 0) + fabs(m<T>(x_first, y_first, 0) - m_m1) * m<T>(x_first, y_first, 1);
    den = fabs(m<T>(x_first, y_first, 2) - m<T>(x_first, y_first, 1)) + fabs(m<T>(x_first, y_first, 0) - m_m1);

    if (den != 0.0)
      b_first[1] = num / den;
    else
      b_first[1] = 0.0;

    for (i = 2; i < (std::size_t)n - 2; i++) {

      num = fabs(m<T>(x_first, y_first, (i + 1)) - m<T>(x_first, y_first, i)) * m<T>(x_first, y_first, (i - 1)) + fabs(m<T>(x_first, y_first, (i - 1)) - m<T>(x_first, y_first, (i - 2))) * m<T>(x_first, y_first, i);
      den = fabs(m<T>(x_first, y_first, (i + 1)) - m<T>(x_first, y_first, i)) + fabs(m<T>(x_first, y_first, (i - 1)) - m<T>(x_first, y_first, (i - 2)));

      if (den != 0.0)
        b_first[i] = num / den;
      else
        b_first[i] = 0.0;
    }

    // i = n - 2
    num = fabs(m_p1 - m<T>(x_first, y_first, (n - 2))) * m<T>(x_first, y_first, (n - 3)) + fabs(m<T>(x_first, y_first, (n - 3)) - m<T>(x_first, y_first, (n - 4))) * m<T>(x_first, y_first, (n - 2));
    den = fabs(m_p1 - m<T>(x_first, y_first, (n - 2))) + fabs(m<T>(x_first, y_first, (n - 3)) - m<T>(x_first, y_first, (n - 4)));

    if (den != 0.0)
      b_first[n - 2] = num / den;
    else
      b_first[n - 2] = 0.0;

    // i = n - 1
    num = fabs(m_p2 - m_p1) * m<T>(x_first, y_first, (n - 2)) + fabs(m<T>(x_first, y_first, (n - 2)) - m<T>(x_first, y_first, (n - 3))) * m_p1;
    den = fabs(m_p2 - m_p1) + fabs(m<T>(x_first, y_first, (n - 2)) - m<T>(x_first, y_first, (n - 3)));

    if (den != 0.0)
      b_first[n - 1] = num / den;
    else
      b_first[n - 1] = 0.0;

    for (i = 0; i < (std::size_t)n - 1; i++) {
      double dxv = x_first[i + 1] - x_first[i];
      c_first[i] = (3 * m<T>(x_first, y_first, i) - 2 * b_first[i] - b_first[i + 1]) / dxv;
      d_first[i] = (b_first[i] + b_first[i + 1] - 2 * m<T>(x_first, y_first, i)) / (dxv * dxv);
    }
  }

  return n;
}

/*! @brief seval calculates a cubic spline for newly given u ("x-axis") and returns a new v ("y-axis")
 *
 * seval v = y(i) + b(i)*(u-x(i)) + c(i)*(u-x(i))**2 + d(i)*(u-x(i))**3<br>
 * where  x(0) < u < x(n), using horner's rule<br>
 * if  u < x(0) then  i = 1  is used<br>
 * if  u > x(n) then  i = n  is used<br>
 * u = the abscissa at which the spline is to be evaluated<br>
 * x,y = the arrays of data abscissas and ordinates<br>
 * b,c,d = arrays of spline coefficients computed by spline / aspline<br>
 *
 * u = target frequency value (from input spectra / not cal spectra)
 * v = output value (interpolation result) for new target frequency u
 * x = frequency list from cal file
 * y = spectral values from cal file
 * b, c, d = coefficients taken priory computed with function aspline (...)
 *
 * @param u new value u
 * @param x_first
 * @param x_last
 * @param y_first
 * @param b_first
 * @param c_first
 * @param d_first
 * @return the interpolated v (u,v)
 */
template <class T, class const_iterator_x>
T seval(const const_iterator_x u,
        const_iterator_x x_first, const const_iterator_x x_last,
        const_iterator_x y_first,
        const_iterator_x b_first,
        const_iterator_x c_first,
        const_iterator_x d_first) {

  std::size_t n = (x_last - x_first);
  std::size_t i, j, k;
  T dx;

  // binary search is performed to determine the proper interval.
  // binary search

  if (*u < x_first[0])
    i = 0;
  else if (*u >= x_first[n - 1])
    i = n - 1;
  else {
    i = 0;
    j = n;

    do {
      k = (i + j) / 2;
      if (*u < x_first[k])
        j = k;
      if (*u >= x_first[k])
        i = k;
    } while (j > i + 1);
  }

  dx = *u - x_first[i];

  return (y_first[i] + dx * (b_first[i] + dx * (c_first[i] + dx * d_first[i])));
}

} // namespace miter

#endif
