.. _FFT:

.. |br| raw:: html

   <br />


FFT / DFT
===========

.. csv-table::
 :header: "name", "alt", "symbol", "unit"
 :delim: ;

 sample rate; sample frequency; f\ :sub:`s`; **Hz** or 1/s
 discretization quantity;; Δt = 1/f\ :sub:`s`; **s**
 samples;; **N**; (no dimension)
 window length;; T = N * Δt; **s**


N corresponds to time measured/recorded or used

2048 pts @1024Hz = 2s = N/fs
1024 pts @1024Hz = 1s
1024 pts @0.0625 (16s) = 16384s

.. csv-table::
 :header: "time Domain", "symbol", "frequency domain", "symbol"
 :delim: ;

 sampling rate; f\ :sub:`s` = 1/Δt; bandwidth; bw = f\ :sub:`s` /2
 samples; N; spectral lines; N/2 + 1 (+1 == DC part)
 window length;T = N * Δt; frequency resolution [Hz]; Δf = bw / N/2 :math:`\frac{1}{0}``

weiter

inline :math::`\frac{1}{0!}+\frac{2}{1!}x+\frac{3}{2!}x^2+\frac{4}{3!}x^3+...` and so on

Cable is 6 mm\ :sup:`2`\  in ...

inline :math:`\frac{1}{0!}+\frac{2}{1!}x+\frac{3}{2!}x^2+\frac{4}{3!}x^3+...` and so on

.. math::

 x = -b \pm \frac{\sqrt{b^{2}-4ac}}{2a}


.. math::
   :name: Fourier transform

   (\mathcal{F}f)(y)
    = \frac{1}{\sqrt{2\pi}^{\ n}}
      \int_{\mathbb{R}^n} f(x)\,
      e^{-\mathrm{i} y \cdot x} \,\mathrm{d} x.

      
