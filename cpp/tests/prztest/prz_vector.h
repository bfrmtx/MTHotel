 
size_t parzen_vector(const std::vector<double> &freqs, const std::vector<double> &target_freqs, const double &prz_radius)
{

    if (!freqs.size() || !target_freqs.size()) return 0;
    const double max_parzen = 0.31;
    if (prz_radius <= 0.0) return 0;
    if (prz_radius >= max_parzen) return 0;


    double prz_rad = prz_radius;
    double step_change = 0.005;

    std::vector<double> selected_freqs;
    std::vector<std::vector<double>::const_iterator >  iter_lwr_bnds;
    std::vector<std::vector<double>::const_iterator >  iter_upr_bnds;
    std::vector<double>::const_iterator low;
    std::vector<double>::const_iterator high;
    std::vector<double>::const_iterator take;

    int max_tries = 40;
    int tries = 0;
/*
    size_t nlines = 3;    // left right

    if (freqs.back() < 0.065) {

        if (freqs.size() < 3 * nlines) return 0;
        auto tf_beg = target_freqs.cbegin();
        auto tf_end = target_freqs.cend();


        auto uf_beg = std::next(freqs.cbegin(), (nlines + 1));
        auto uf_end = std::prev(freqs.cend(), (nlines + 1));

        tf_beg =  std::lower_bound(tf_beg, tf_end, *freqs.cbegin()) ;




        for (; tf_beg != tf_end; ++tf_beg) {
            low =  std::lower_bound(uf_beg, uf_end, *tf_beg) ;
           std::cout << *low << *tf_beg << *(std::next(low, 2));
            if ( (low != uf_end) && (low != uf_beg)) {
                take = std::prev(low, 1);
                if (*low == *tf_beg) {
                    high = std::next(low, nlines);
                    low = std::prev(low, nlines);
                    selected_freqs.push_back(*tf_beg);
                    iter_lwr_bnds.push_back(std::prev(low, nlines));
                    iter_upr_bnds.push_back(std::next(high, nlines));

                   std::cout << "taken" << *(iter_lwr_bnds.back()) << *tf_beg << *(iter_upr_bnds.back());


                }
                else if ((*low > *tf_beg) && ( *take < *tf_beg)) {
                    high = std::next(low, (nlines -1));
                    take = std::prev(low, 1);
                    selected_freqs.push_back(*tf_beg);
                    iter_lwr_bnds.push_back(take);
                    iter_upr_bnds.push_back(high);
                   std::cout << "taken" << *(iter_lwr_bnds.back()) << *tf_beg << *(iter_upr_bnds.back());


                }
            }
            // high = std::upper_bound(uf_beg, uf_end, *tf_beg) ; this will ONLY work if target frequency is REALLY inside


        }

       std::cout << "line selector used";


        //        auto uf_beg = std::next(freqs.cbegin(), (nlines + 2));
        //        auto uf_end = std::prev(freqs.cend(), (nlines + 1));

        //        for (; uf_beg != uf_end; ++uf_beg) {
        //            low =  std::lower_bound(target_freqs.cbegin(), target_freqs.cend(), *uf_beg) ;
        //            high = std::upper_bound(target_freqs.cbegin(), target_freqs.cend(), *uf_beg) ;
        //            if (std::distance(low, high) > nlines) {
        //                selected_freqs.push_back(*low);
        //                iter_lwr_bnds.push_back(std::prev(low, nlines);
        //                iter_upr_bnds.push_back(std::next(high, nlines));
        //            }

        //        }

       std::cout << "line selector used";



        //        iter_lwr_bnds.reserve(freqs.size()-2);
        //        selected_freqs.reserve(freqs.size()-2);
        //        iter_upr_bnds.reserve(freqs.size()-2);

        //        auto fbeg = freqs.cbegin() + 3;
        //        auto fend = freqs.cend() - 4;

        //        if (std::distance(fbeg, fend) > 3) {
        //            while (fbeg != fend) {
        //                if (std::find(target_freqs.begin(), target_freqs.end(), *fbeg) != target_freqs.end() ) {
        //                    iter_lwr_bnds.push_back(fbeg-1);
        //                    selected_freqs.push_back(*fbeg);
        //                    iter_upr_bnds.push_back(fbeg+1);
        //                }
        //                ++fbeg;
        //            }
        //        }


    }

    else do {
 */
    do {
        if (prz_rad < max_parzen) {
            selected_freqs.clear();
            iter_lwr_bnds.clear();
            iter_upr_bnds.clear();
            std::vector<double>::const_iterator  min_freq = freqs.begin();
            std::vector<double>::const_iterator  max_freq = freqs.end()-1;

            // eg. the zero element contains the DC part f = 0 Hz
            if (*min_freq == 0.0) ++min_freq;

            // copy the list
            std::vector<double> lower_prz_bounds = target_freqs;
            std::vector<double> upper_prz_bounds = target_freqs;


            //set the range of frequencies to be included in pazening
            // so if target f was 100 and prz_rad = 0.1 we have 90 .. 100 .. 110
            for ( size_t i = 0; i < lower_prz_bounds.size(); ++ i) {
                lower_prz_bounds[i] = lower_prz_bounds[i] - (lower_prz_bounds[i] * prz_rad);
                upper_prz_bounds[i] = upper_prz_bounds[i] + (upper_prz_bounds[i] * prz_rad);
            }

            // now find the itrators pointing to the valid areas of the frequency vector containing the parzen freqs
            // *iter_lwr_bnds[i] is value, iter_lwr_bnds - f.begin() is pos
            for (size_t i = 0; i < lower_prz_bounds.size(); ++i) {
                if ((lower_prz_bounds[i] >= *min_freq) && (upper_prz_bounds[i] <= *max_freq) ) {
                    low =  std::lower_bound(min_freq, max_freq, lower_prz_bounds[i]) ;
                    high = std::upper_bound(min_freq, max_freq, upper_prz_bounds[i]) ;
                   std::cout << *high  << " <-> "   << *low << std::endl;
                   std::cout << 1.0 / (*high) << " <-> " << 1.0 / (*low) << " target: " << 1.0 / target_freqs[i] << std::endl;
                    if (high - low > 1) {
                        // all lower start points
                        iter_lwr_bnds.push_back(low);
                        // all upper end points
                        iter_upr_bnds.push_back(high);
                        // all freqs belong to that
                        selected_freqs.push_back(target_freqs[i]);
                    }
                    else {
                       std::cout << "slot_parzen_vector -> no bounds" << lower_prz_bounds[i] << *min_freq << upper_prz_bounds[i] << *max_freq << std::endl;
                       std::cout << "slot_parzen_vector -> no bounds" << 1.0  / lower_prz_bounds[i] << 1.0/ (*min_freq) << 1.0 / upper_prz_bounds[i] << 1.0 / (*max_freq) << std::endl;

                    }
                }

            }

            ++tries;
            if(selected_freqs.size() < 1) {
                prz_rad += step_change;
                std::cerr << " math_vector::slot_parzen_vector ---> changed PARZEN! " << prz_rad << std::endl;
            }
        }

    } while ((selected_freqs.size() < 1) && (tries < max_tries));

    if (prz_rad != prz_radius) {
        emit this->parzen_radius_changed(prz_radius, prz_rad);
    }

    emit this->frequencies_selected(selected_freqs);



    std::vector<double> parzendists;

    // I must notify my mother
    if (!selected_freqs.size()) {
        std::cerr << " math_vector::slot_parzen_vector FAILED!!!" << std::endl;
        emit this->parzenvector_created(0, parzendists);
        return 0;
    }

    parzendists.resize(freqs.size());

    double dist = 0;
    double distsum = 0;
    size_t k;

    // for each of the frequencies found we make a parzen vector (with length of the incoming fft spectra freqs
    for ( size_t i = 0; i < selected_freqs.size(); ++i) {
        // no realloc here; reset to 0
        parzendists.assign(freqs.size(), 0.0);
        distsum = 0;
        dist = 0;
        low = iter_lwr_bnds[i];  // save the original iterpos
        high = iter_upr_bnds[i];
        while (low < high) {
            k = (low - freqs.begin()); // index of frequency vector is naturally the same as freq
            dist = ( abs(selected_freqs[i] - *low) * M_PI) / (selected_freqs[i] * prz_rad);
            // k is the index of freqs
            if (!dist) parzendists[k] = 1.0;
            else {
                dist = pow( ((sin(dist))/dist), 4.);
                //dist = ((sin(dist))/dist);
                parzendists[k] = dist;

            }
            distsum += dist;
            ++low;
            //if (dist > 0.0001) this->mtd->avgfs[i] += 1;
        }
        //this->mtd->bws[i] =  (this->mtd->avgfs[i] * this->mcd->get_key_dvalue("sample_freq")) / ( this->mcd->get_key_uivalue("read_samples") * M_PI);
        //      std::cout << this->mtd->avgfs[i] << this->mcd->get_key_dvalue("sample_freq") << this->mcd->get_key_uivalue("read_samples");
        //      std::cout << distsum << "at" << this->mtd->przfreq_sel.at(i) << *iter_lwr_bnds[i] << *iter_upr_bnds[i] << *iter_upr_bnds[i] - *iter_lwr_bnds[i] << j << "lines";
        for ( auto &val : parzendists) val /= distsum;
        emit this->parzenvector_created(selected_freqs.at(i), parzendists);
    }

    return 1;
}
