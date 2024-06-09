#include "spc_base.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

#include "vector_math.h"

void mul_vec(std::vector<double> &vec, const double &mul) {
  // deep copy of vec
  std::vector<double> vec_in = vec;
  for (size_t i = 0; i < vec.size(); ++i) {
    std::cout << "mul_vec: " << vec[i] << " * " << mul << std::endl;
    vec[i] *= mul;
  }
  std::cout << "result of mul_vec" << std::endl;
  for (size_t i = 0; i < vec.size(); ++i) {
    std::cout << vec_in[i] << " -> " << vec[i] << std::endl;
  }
}

std::unordered_map<std::string, size_t> seat_reservation(const std::vector<std::string> &names, size_t seats) {

  // seats we use it to force an error ans see how the function and main behaves
  std::unordered_map<std::string, size_t> reservation;

  try {
    if (names.size() > seats) {
      // std::string err_str = __func__;
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::can not make reservation: seats: " << seats << ", persons: " << names.size();
      throw std::runtime_error(err_str.str());
    }
    // so seat ok, continue
    size_t i = 0;
    for (const auto &name : names) {
      reservation.emplace(name, i++);
    }

  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    std::cout << "function call reservation WITH catched exception" << std::endl;
    std::cerr << "making a fake reservation" << std::endl;

    // so this code will be executed after throw
    // reserve what I have - but I have to ask for names later
    for (size_t i = 0; i < seats; ++i) {
      reservation.emplace("reserved_seat_" + std::to_string(i), i);
    }
  }
  // that would catch others
  catch (...) {
  }

  // that is finally returned
  return reservation;
}

std::unordered_map<std::string, size_t> seat_reservation_wo(const std::vector<std::string> &names, size_t seats) {

  // seats we use it to force an error ans see how the function and main behaves
  std::unordered_map<std::string, size_t> reservation;

  if (names.size() > seats) {
    // std::string err_str = __func__;
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::can not make reservation: seats: " << seats << ", persons: " << names.size();
    std::cout << "function call reservation WITHOUT catched exception" << std::endl;
    throw std::runtime_error(err_str.str());
  }
  // so seat ok, continue
  size_t i = 0;
  for (const auto &name : names) {
    reservation.emplace(name, i++);
  }

  // that is finally returned
  return reservation;
}

int main() {

  std::vector<std::string> persons;
  std::vector<size_t> seats{3, 3};

  persons.emplace_back("Billy");
  persons.emplace_back("Ted");
  persons.emplace_back("Scooter");

  std::unordered_map<std::string, size_t> result;

  for (size_t count = 0; count < 2; ++count) {
    for (const auto &seat : seats) {

      try {
        if (!count) {
          std::cout << "starting reservation MAIN, WITH exeception catched INSIDE function: " << std::endl;
          result = seat_reservation(persons, seat);
        } else {
          std::cout << "starting reservation MAIN, WITHOUT exeception catched INSIDE function: " << std::endl;
          result = seat_reservation_wo(persons, seat);
        }

        if (!result.size()) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << ":: can not make reservation - msg from MAIN";
          throw std::runtime_error(err_str.str());
        } else {
          if (result.size() == persons.size())
            std::cout << "MAIN reservation successful" << std::endl;
          else
            std::cout << "MAIN reservation PROBLEM, solved" << std::endl;
          for (const auto &p : result) {
            std::cout << p.first << " " << p.second << std::endl;
          }
        }

      } catch (const std::runtime_error &error) {
        std::cerr << error.what() << std::endl;
        std::cerr << "MAIN could not execute reservation in MAIN" << std::endl;
      }
      std::cout << std::endl;

      result.clear();
    }
  }
  std::cout << std::endl;
  std::cout << "*************************************************" << std::endl;

  std::cout << std::endl;
  std::cout << "************************no catch in main -> program dies *************************" << std::endl;
  std::cout << "throwing an instance of 'std::__cxx11::basic_string" << std::endl;
  std::cout << "will be the error message, because we throw a string" << std::endl
            << std::endl;

  for (const auto &seat : seats) {

    std::cout << "starting reservation MAIN, WITHOUT exeception catched INSIDE function: " << std::endl;
    result = seat_reservation_wo(persons, seat);
    if (!result.size()) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: can not make reservation - msg from MAIN";
      throw std::runtime_error(err_str.str());
    } else {
      if (result.size() == persons.size())
        std::cout << "MAIN reservation successful" << std::endl;
      else
        std::cout << "MAIN reservation PROBLEM, solved" << std::endl;
      for (const auto &p : result) {
        std::cout << p.first << " " << p.second << std::endl;
      }
    }

    std::cout << std::endl;

    result.clear();
  }

  auto vec = std::make_shared<std::vector<double>>(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0});
  mul_vec(*vec, 2.0);

  std::cout << "**************************************************************" << std::endl;
  spc_base<double> spc;
  std::vector<double> vec1{1.0, 2.0, 3.0, 4.0, 5.0};
  // spc.add_spectra("Ex", std::move(vec1), 0.0);
  std::pair<std::string, std::string> channel{"Ex", ""};
  spc.add_spectra(channel.first, vec1, 0.0, true);
  std::cout << "main moved vec1 " << std::endl;
  for (const auto &v : vec1) {
    std::cout << v << std::endl;
  }
  std::cout << "main inside spc " << std::endl;
  for (const auto &v : spc.get_spectra_vec(channel)) {
    std::cout << v << std::endl;
  }
  auto vec2 = spc.get_spectra_vec(channel);
  for (auto &v : vec2) {
    v *= 2.0;
  }
  std::cout << "main inside spc after MULt vec2" << std::endl;
  for (const auto &v : spc.get_spectra_vec(channel)) {
    std::cout << v << std::endl;
  }
  std::cout << "now using shared pointer" << std::endl;
  auto vec3 = spc.get_spectra(channel);
  for (auto &v : *vec3) {
    v *= 2.0;
  }
  std::cout << "main inside spc after MULt vec3" << std::endl;
  for (const auto &v : spc.get_spectra_vec(channel)) {
    std::cout << v << std::endl;
  }
  std::cout << "now use the map in order to get the vector" << std::endl;
  auto vec4 = spc[channel];
  for (auto &v : *vec4) {
    v *= 2.0;
  }
  std::cout << "main inside spc after MULt vec4" << std::endl;
  for (const auto &v : spc.get_spectra_vec(channel)) {
    std::cout << v << std::endl;
  }
  std::cout << "try to create a conventional vector" << std::endl;
  std::vector<double> vec5 = *spc[channel]; // that is a COPY !!
  for (auto &v : vec5) {
    v *= 2.0;
  }
  std::cout << "main inside spc after MULt vec5" << std::endl;
  for (const auto &v : spc.get_spectra_vec(channel)) {
    std::cout << v << std::endl;
  }
  std::cout << "try with a function" << std::endl;
  mul_vec(*spc[channel], 2.0);
  std::cout << "check again, inside spc after MULt vec5" << std::endl;
  for (const auto &v : *spc[channel]) {
    std::cout << v << std::endl;
  }
  std::cout << "another trick with & operator" << std::endl;
  auto &vec6 = *spc[channel];
  for (auto &v : vec6) {
    v *= 2.0;
  }
  std::cout << "main inside spc after MULt vec6" << std::endl;
  for (const auto &v : spc.get_spectra_vec(channel)) {
    std::cout << v << std::endl;
  }

  std::cout << "tesing vector_math merge_f_v_avg" << std::endl;
  std::vector<double> v_result;
  std::vector<double> f_result;

  std::vector<double> f_1{1.0, 2.0, 3.0, 4.0, 5.0};
  std::vector<double> v_1{10.0, 20.0, 30.0, 40.0, 50.0};

  std::vector<double> f_2{2.0, 3.0, 4.0, 7.0, 9.0};
  std::vector<double> v_2{2.0, 3.0, 4.0, 7.0, 9.0};

  bvec::merge_f_v_avg(f_1, v_1, f_2, v_2, f_result, v_result);
  std::cout << "f_result, v_result " << std::endl;
  for (size_t i = 0; i < f_result.size(); ++i) {
    std::cout << f_result[i] << " " << v_result[i] << std::endl;
  }

  std::cout << "main finished " << std::endl;
  return 0;
}
