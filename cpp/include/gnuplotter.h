#ifndef GNUPLOTTER_H
#define GNUPLOTTER_H

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

class gnuplotter
{
public:
    gnuplotter(const int device) :device(device) {

        // windows maybe _popen
        this->plotHandle = popen("gnuplot -persist\n", "w");
        if(this->plotHandle == nullptr) {
            std::string err_str = __func__;
            err_str += "::can not open gnuplot pipe!";
            throw err_str;
        }

    }

    ~gnuplotter() {
        if(this->plotHandle != nullptr) {
            pclose(this->plotHandle);
        }
        // do I explictly delete this
        // I get free(): double free detected in tcache 2
        // delete this->plotHandle;
    }

    void plot() {
        if (this->device == 1) {
            fprintf(plotHandle, "%s", this->cmd.str().c_str());

            stmp << "plot ";
            size_t i = 0;
            for (const auto &str : this->plt) {
                if (i && (i < this->plt.size())) stmp << ", ";
                stmp << str;
                ++i;
            }
            // stmp << std::endl; ;
            fprintf(plotHandle, "%s", stmp.str().c_str());
            fprintf(plotHandle, "\n");

            if (this->data.tellp() > 1) {
                fprintf(plotHandle, "%s", this->data.str().c_str());
            }

            fflush(plotHandle);

            //fprintf(plotHandle, "%s", this->data.str().c_str());
        }

        if (this->device == 2) {

        }

        if (this->device == 3) {

        }


    }

    void set_x_range(const double &xmin, const double &xmax) {

        this->cmd << "set xrange [" << xmin << ":" << xmax << "]" << std::endl;
    }

    void set_xy_points(const std::vector<double> &x, const std::vector<double> &y, const std::string title, const std::uint64_t size, const std::string formats ="") {

        std::stringstream tmp;
        tmp << "'-' using 1:2 with points pointsize " << size;
        if (formats.size()) tmp << " " << formats;
        if (title.size())  tmp << " title '" << title << "'";        plt.push_back(tmp.str());
        this->set_data(x,y);
    }

    void set_xy_lines(const std::vector<double> &x, const std::vector<double> &y, const std::string title, const std::uint64_t size, const std::string formats ="") {

        std::stringstream tmp;
        tmp << "'-' using 1:2 with lines linewidth " << size;
        if (formats.size()) tmp << " " << formats;
        if (title.size())  tmp << " title '" << title << "'";        plt.push_back(tmp.str());
        this->set_data(x,y);
    }

    void set_xy_linespoints(const std::vector<double> &x, const std::vector<double> &y, const std::string title, const std::uint64_t size, const std::string formats ="") {

        std::stringstream tmp;
        tmp << "'-' using 1:2 with linespoints linewidth " << size;
        if (formats.size()) tmp << " " << formats;
        if (title.size())  tmp << " title '" << title << "'";
        plt.push_back(tmp.str());
        this->set_data(x,y);
    }

    std::stringstream data;
    std::stringstream cmd;
    std::vector<std::string> plt;
    std::stringstream stmp;



private:

    FILE *plotHandle = nullptr;
    const int device = 0; // 1 screen, 2 file background, 3 = file png

    void set_data (const std::vector<double> &x, const std::vector<double> &y) {

        if (x.size() != y.size()) {
            std::string err_str = __func__;
            err_str += "::vectors have different sizes!";
            throw err_str;
            return;
        }

        this->data << std::scientific;

        for (size_t i = 0; i < x.size(); ++i) {
            this->data << x[i] << " " << y[i] << std::endl;
        }

        this->data << "e" << std::endl;
    }


};


#endif
