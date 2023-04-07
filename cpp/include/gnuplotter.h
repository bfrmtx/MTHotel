#ifndef GNUPLOTTER_H
#define GNUPLOTTER_H

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ostream>
#include <string>
#include <vector>
#include <queue>
#include <utility>
#include <type_traits>
#include <filesystem>


//### plot timedata equidistant as xtic label
//reset session

//$Data <<EOD
//1609257628 2
//1609256682 4
//1609255914 1
//EOD

//myTimeFmt = "%d.%m.%Y %H:%M"

//plot $Data u 0:2:xtic(strftime(myTimeFmt,column(1))) w lp pt 7
//### end of code


template <class T, class S>
class gnuplotter
{
public:
    /*!
     * \brief gnuplotter
     * \param err_str returns the error in initialization; you should return EXIT_FAILURE in main
     * \param outfile_ writes a gnuplot script (with binary data) to script file; open with gnuplot -persist sprectra.bgp for example
     *
     * make a script file
     *   auto gplt_mm = std::make_unique<gnuplotter<double, double>>(init_err_mm, "/tmp/x.bgp");
     *   gplt_mm->cmd << "set terminal qt size 2048,1600 enhanced" << std::endl;
     *
     *   later you can use it as gnuplot -persist /tmp/x.bgp
     *
     * make a plot on screen - leave the filename empty
     *    auto gplt_mm = std::make_unique<gnuplotter<double, double>>(init_err_mm);
     *    gplt_mm->cmd << "set terminal qt size 2048,1600 enhanced" << std::endl;
     *
     * make a plot into file - leave the filename empty but use the output option
     *    auto gplt_mm = std::make_unique<gnuplotter<double, double>>(init_err_mm);
     *    gplt_mm->cmd << "set terminal svg size 2048,1600 enhanced" << std::endl;
     *    gplt_mm->cmd << "set output '/tmp/spectra.svg'" << std::endl;
     */
    gnuplotter(std::string &err_str, const std::string outfile_ = "") {

        try {
            if (outfile_.size()) {
                this->outfile = std::filesystem::path(outfile_);
                this->plotHandle = fopen(outfile_.c_str(), "w");

            } else {
                // windows maybe _popen
                this->plotHandle = popen("gnuplot -persist\n", "w");
            }
        }
        catch(const std::exception& ex) {
            err_str = __func__;
            err_str+= " canonical path for gnuplot: " + outfile_ + err_str + " ::" + ex.what();
            return;
        }

        if (this->plotHandle == nullptr) {
            err_str = __func__;
            if (!this->outfile.empty()) err_str += "::can not open gnuplot file!";
            else err_str += "::can not open gnuplot pipe!";
            return;
        }

        this->data_format << "format='";
        if (typeid(T) == typeid(double)) this->data_format << "%float64";
        else if (typeid(T) == typeid(float)) this->data_format << "%float32";
        else if (typeid(T) == typeid(int32_t)) this->data_format << "%int32";
        else if (typeid(T) == typeid(int64_t)) this->data_format << "%int64";
        else {
            err_str = __func__;
            err_str += "::xaxis can be double, float, int32_t or int64_t ONLY";
        }
        if (typeid(S) == typeid(double)) this->data_format << "%float64";
        else if (typeid(S) == typeid(float)) this->data_format << "%int32";
        else if (typeid(S) == typeid(int32_t)) this->data_format << "%int32";
        else if (typeid(S) == typeid(int64_t)) this->data_format << "%int64";
        else {
            std::string err_str = __func__;
            err_str += "::yaxis can be double, float, int32_t or int64_t ONLY";
        }
        this->data_format << "'";
    }

    ~gnuplotter() {
        if(this->plotHandle != nullptr) {
            if (this->outfile.empty()) pclose(this->plotHandle);
            else fclose(this->plotHandle);
        }
        // do I explictly delete this
        // I get free(): double free detected in tcache 2
        // delete this->plotHandle;
    }

    void plot() {
        std::ostringstream stmp;

        fprintf(this->plotHandle, "%s", this->cmd.str().c_str());
        stmp << "plot ";
        size_t i = 0;
        for (const auto &str : this->plt) {
            if (i && (i < this->plt.size())) stmp << ", ";
            stmp << str;
            ++i;
        }
        if (plt.size()) {
            fprintf(this->plotHandle, "%s", stmp.str().c_str());
            fprintf(this->plotHandle, "\n");
        }
        plt.clear();
        if (this->xaxis.size() && (this->xaxis.size() == this->yaxis.size()) ) {
            while (!this->xaxis.empty()) {
                auto &x = this->xaxis.front();
                auto &y = this->yaxis.front();
                for (size_t i = 0; i < x.size(); ++i) {
                    fwrite(&x[i], sizeof(char), sizeof(T), this->plotHandle);
                    fwrite(&y[i], sizeof(char), sizeof(S), this->plotHandle);
                }
                this->xaxis.pop();
                this->yaxis.pop();
            }
        }
        fprintf(this->plotHandle, "\n");
        fflush(this->plotHandle);
        // clear commands
        this->cmd = std::ostringstream();
    }

    void set_x_range(const double &xmin, const double &xmax) {

        this->cmd << "set xrange [" << xmin << ":" << xmax << "]" << std::endl;
    }

    void set_x_range(const std::pair<double, double> &min_max) {

        this->cmd << "set xrange [" << min_max.first << ":" << min_max.second << "]" << std::endl;
    }

    void set_y_range(const std::pair<double, double> &min_max) {

        this->cmd << "yrange [" << min_max.first << ":" << min_max.second << "]" << std::endl;
    }

    void set_y_range(const double &ymin, const double &ymax) {

        this->cmd << "set yrange [" << ymin << ":" << ymax << "]" << std::endl;
    }

    void set_qt_terminal(const std::string &name = "", const size_t &file_count = 0) {
        this->cmd << "set terminal qt size 2048,1536 enhanced font 'Noto Sans, 10'";
        if (name.size()) this->cmd << " title '" << name;
        if (file_count)  this->cmd << "_" << file_count;
        else this->cmd<< "'";
        this->cmd << std::endl;
    }

    void set_svg_terminal(const std::filesystem::path &path, const std::string &name, const size_t &max_sizes, const size_t &file_count = 0) {
        std::ostringstream nam;

        this->cmd << "set terminal svg size 1024,768 dynamic enhanced font 'Noto Sans, 14' name '" << name << "'"  << std::endl;
        if (!file_count) nam << "set output '" <<  path.string() << "/" << name <<  ".svg'";
        else nam << "set output '" <<  path.string() << "/" << name << "_" << file_count << ".svg'";
        this->cmd << nam.str() << std::endl;

        this->max_sizes = max_sizes;  // take 1 for svg - lines and symbol thickness are bad
    }



    void set_xy_points(const std::vector<T> &x, const std::vector<S> &y, const std::string title, const std::uint64_t ps, const std::string formats ="") {

        this->set_data(x,y);
        std::ostringstream tmp;
        tmp << "'-' binary record=(" << x.size() << ") " << this->data_format.str() << " with points ps " << ps;
        if (formats.size()) tmp << " " << formats;
        if (title.size())  tmp << " title '" << title << "'";
        this->plt.push_back(this->trim(tmp.str()));
    }

    void set_xy_lines(const std::vector<T> &x, const std::vector<S> &y, const std::string title, const std::uint64_t lw, const std::string formats ="") {

        this->set_data(x,y);
        std::ostringstream tmp;
        tmp << "'-' binary record=(" << x.size() << ") " << this->data_format.str() << " with lines" << this->set_lw(lw);
        if (formats.size()) tmp << " " << formats;
        if (title.size())  tmp << " title '" << title << "'";
        this->plt.push_back(this->trim(tmp.str()));
    }

    void set_xy_linespoints(const std::vector<T> &x, const std::vector<S> &y, const std::string &title, const std::uint64_t &lw, const std::uint64_t &ps, const std::string &formats ="") {

        this->set_data(x,y);
        std::ostringstream tmp;
        tmp << "'-' binary record=(" << x.size() << ") " << this->data_format.str() << " with linespoints" << this->set_lw(lw) << this->set_ps(ps);
        if (formats.size()) tmp << " " << formats;
        if (title.size())  tmp << " title '" << title << "'";
        this->plt.push_back(this->trim(tmp.str()));
    }

    std::ostringstream cmd;
    std::vector<std::string> plt;

    std::ostringstream data_format;

    std::queue<std::vector<T>> xaxis;
    std::queue<std::vector<S>> yaxis;





private:

    FILE *plotHandle = nullptr;
    std::filesystem::path outfile;

    uint64_t max_sizes = 100;

    std::string set_lw(const uint64_t &lw) {
        std::ostringstream tmp;
        if (lw > this->max_sizes) tmp << " lw " << max_sizes;
        else tmp  << " lw " << lw;
        return tmp.str();
    }

    std::string set_ps(const uint64_t &ps) {
        std::ostringstream tmp;
        if (ps > this->max_sizes) tmp << " ps " << max_sizes;
        else tmp  << " ps " << ps;
        return tmp.str();
    }

    void set_data(const std::vector<T> &x, const std::vector<S> &y) {

        if (x.size() != y.size()) {
            std::string err_str = __func__;
            err_str += "::vectors have different sizes!";
            throw err_str;
            return;
        }
        this->xaxis.push(std::vector<T>(x));
        this->yaxis.push(std::vector<S>(y));
    }

    void ascci_blocks() {
        if (this->xaxis.size() && (this->xaxis.size() == this->yaxis.size()) ) {
            while (!this->xaxis.empty()) {
                this->data.clear();
                this->data.str().clear();
                this->data << std::scientific;
                auto &x = this->xaxis.front();
                auto &y = this->yaxis.front();

                for (size_t i = 0; i < x.size(); ++i) {
                    this->data << x[i] << " " << y[i] << std::endl;
                }
                this->data << "e" << std::endl;
                if (this->data.tellp() > 1) {
                    fprintf(this->plotHandle, "%s", this->data.str().c_str());
                }
                this->xaxis.pop();
                this->yaxis.pop();
            }
        }
        this->data.clear();
        this->data.str().clear();
    }

    // copy from my library - this plotter may wants to be copied without dependencies

    /*!
     * \brief rtrim remove trailing empty spaces from a string
     * \param s
     * \return
     */
    std::string rtrim(const std::string& s) {
        std::string ws(" \t\f\v\n\r");
        auto found = s.find_last_not_of(ws);
        if (found != std::string::npos) {
            return s.substr(0, found + 1);
        }
        return s;
    }

    /*!
     * \brief ltrim remove leading empty spaces from a string
     * \param s
     * \return
     */
    std::string ltrim(const std::string& s) {
        std::string ws(" \t\f\v\n\r");
        auto found = s.find_first_not_of(ws);
        if (found != std::string::npos) {
            return s.substr(found);
        }
        return std::string();
    }

    /*!
     * \brief trim trims a string on both ends
     * \param s
     * \return
     */
    std::string trim(const std::string& s) {
        std::string str(rtrim(s));
        return ltrim(str);
    }


};


/*
    std::string init_err_mm;
    auto gplt_mm = std::make_unique<gnuplotter<double, double>>(init_err_mm);

    if (init_err_mm.size()) {
        std::cout << init_err_mm << std::endl;
        return EXIT_FAILURE;
    }


    gplt_mm->cmd << "set terminal qt size 2048,1600" << std::endl;

//    gplt_mm->cmd << "set terminal svg size 2048,1600" << std::endl;
//    gplt_mm->cmd << "set output '/tmp/spectra.svg'" << std::endl;
    gplt_mm->cmd << "set multiplot layout 2, 1" << std::endl;
    gplt_mm->cmd << "set title 'TS Data'" << std::endl;
    //gplt->cmd << "set key off" << std::endl;
    gplt_mm->cmd << "set xlabel 'ts'" << std::endl;
    gplt_mm->cmd << "set ylabel 'amplitude [mV]'" << std::endl;
    gplt_mm->cmd << "set grid" << std::endl;
    gplt_mm->cmd << "set key font \"Hack, 10\"" << std::endl;
    //gplt_mm->cmd << "plot sin(x)/x" << std::endl;
    gplt_mm->set_x_range(ts_beg, ts_end);
    gplt_mm->set_xy_lines(xax, yax, "input", 1);

    gplt_mm->plot();

    //gplt_mm->cmd << std::endl;
    gplt_mm->cmd << "set title 'TS 2x Data'" << std::endl;
    //gplt->cmd << "set key off" << std::endl;
    gplt_mm->cmd << "set xlabel 'ts'" << std::endl;
    gplt_mm->cmd << "set ylabel 'amplitude [mV]'" << std::endl;
    gplt_mm->cmd << "set grid" << std::endl;
    gplt_mm->cmd << "set key font \"Hack, 10\"" << std::endl;

    gplt_mm->set_x_range(ts_beg, ts_end);
    for(auto &y : yax) y *= 10.0;
    gplt_mm->set_xy_lines(xax, yax, "input", 1);
    //gplt_mm->cmd << "plot sin(x)" << std::endl;

    gplt_mm->plot();


*/


#endif
