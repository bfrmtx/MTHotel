#ifndef SPC_BASE_H
#define SPC_BASE_H
#include <algorithm>
#include <complex>
#include <iostream>
#include <map>
#include <memory>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "atss.h"
#include "base_constants.h"
#include "cal_base.h"

/**
 * @brief A class template for managing spectra. T is at least a std::vector<T> or std::vector<std::vector<T>>.
 *
 * This class is derived from std::map and provides functionality for adding, moving, retrieving, and deleting spectra.
 * Each  spectrum is associated with a name and stored as a shared pointer to a vector or a vector of vectors.
 * The class also allows setting and getting the bandwidth of the spectra.
 *
 * @tparam T The type of the elements in the spectra vector, like double or std::complex<double> OR <std::vector<std::complex<double>>>, <std::vector<std::double>>
 */
template <typename T>
class spc_base : public std::map<std::pair<std::string, std::string>, std::shared_ptr<std::vector<T>>> {
public:
  spc_base() = default;
  ~spc_base() = default;

  // ******************************************************  M O V I N G  / A D D I N G   S P E C T R A  ****************************************************************
  // 1) ********* for shared pointers ************
  /**
   * @brief Moves a single spectrum to the collection, e.g. from a channel object
   * @param name_in The name of the spectrum. If name is a single string, the second name is empty so Hx -> Hx and <Hx, Hy> -> <Hx, Hy>
   * @param spectra The vector containing the spectral data. It will be **** MOVED **** into the map. if spectra is  N U L L P T R  , an empty vector will be created
   * @throws std::runtime_error if a spectrum with the same name already exists
   */
  template <typename S>
  void add_spectra(const S &name_in, std::shared_ptr<std::vector<T>> spectra = nullptr, const double &bw = 0.0, const bool move_spc = false) {
    std::shared_lock lock(spc_lock);
    std::pair<std::string, std::string> name;
    if constexpr (std::is_same_v<S, std::pair<std::string, std::string>>) {
      name = name_in;
    } else if constexpr (std::is_same_v<S, std::string>) {
      name = std::pair<std::string, std::string>(name_in, "");
    } else {
      throw std::runtime_error("spc_base::add_spectra: unknown type of name");
    }
    if (this->find(name) != this->end()) {
      if (move_spc)
        throw std::runtime_error("spc_base::add_spectra (move): spectra with name " + this->get_name(name) + " already exists");
      else
        throw std::runtime_error("spc_base::add_spectra: spectra with name " + this->get_name(name) + " already exists");
    }
    if (spectra == nullptr) { // we use a shared pointer later, so we need to create an empty vector
      this->emplace(name, std::make_shared<std::vector<T>>());
    } else if (move_spc)
      this->emplace(name, std::move(spectra));
    else
      this->emplace(name, spectra);
    if (bw != 0.0)
      this->bw = bw;
  }
  /*!
   * @brief same as above, convience function for two strings
   */
  void add_spectra(const std::string &name_in, const std::string &name_in2, std::shared_ptr<std::vector<T>> spectra = nullptr, const double &bw = 0.0, const bool move_spc = false) {
    auto name = std::pair<std::string, std::string>(name_in, name_in2);
    this->add_spectra(name, spectra, bw, move_spc);
  }

  // 2) ********* for standard vectors ************
  /*!
   * @brief same as above, but we have a vector, not a shared pointer; we make a shared pointer and move it
   */
  template <typename S>
  void add_spectra(const S &name_in, std::vector<T> &spectra, const double &bw = 0.0, const bool move_spc = false) {
    if (move_spc)
      this->add_spectra(name_in, std::make_shared<std::vector<T>>(std::move(spectra)), bw, true);
    else
      this->add_spectra(name_in, std::make_shared<std::vector<T>>(spectra), bw, false);
  }

  /*!
   * @brief same as above, convience function for two strings
   */
  void add_spectra(const std::string &name_in, const std::string &name_in2, std::vector<T> &spectra, const double &bw = 0.0, const bool move_spc = false) {
    auto name = std::pair<std::string, std::string>(name_in, name_in2);
    this->add_spectra(name, spectra, bw, move_spc);
  }

  // 3) ************************* move a channel object to the collection ************************
  /*!
   * @brief this is called for a channel object, we ALWAYS move the spectrum from the channel to the collection; channel has a shared pointer!
   * @param chan the channel object can be set to be remote or emap; this will be considered
   */
  void add_spectra(std::shared_ptr<channel> chan) {
    std::string name_in = chan->channel_type;
    if (is_E(name_in) && chan->is_emap && !chan->is_remote) {
      name_in = "E" + name_in;
    }
    if (!chan->is_emap && chan->is_remote) {
      name_in = "R" + name_in;
    }
    auto name = std::pair<std::string, std::string>(name_in, "");
    this->add_spectra(name, chan->spc, chan->bw, true);
  }

  // ******************************************************  R E T R I E V I N G   S P E C T R A  ****************************************************************
  // 1) as shared pointer
  /**
   * @brief Retrieves a single spectrum from the collection.
   * @param name The name of the spectrum to retrieve. If this spectra has been collected already, and you want to be it remote or emap, you can set the flags
   * @param is_remote if you ask this spectra, and the channel is Hx, it will return RHx, if it is set to be remote
   * @param is_emap if you ask this spectra, and the channel is Hx, it will return EHx, if it is set to be emap
   * @return A shared pointer to the vector containing the spectrum data.
   * @throws std::runtime_error if the spectrum with the given name does not exist.
   */
  template <typename S>
  std::shared_ptr<std::vector<T>> get_spectra(const S &name_out, const bool is_remote = false, const bool is_emap = false) const {
    std::shared_lock lock(spc_lock);
    std::pair<std::string, std::string> name;
    if constexpr (std::is_same_v<S, std::pair<std::string, std::string>>) {
      name = name_out;
    } else if constexpr (std::is_same_v<S, std::string>) {
      name = std::pair<std::string, std::string>(name_out, "");
      if (is_E(name.first) && is_emap && !is_remote) {
        name.first = "E" + name.first;
      }
      if (!is_emap && is_remote) {
        name.first = "R" + name.first;
      }
    } else {
      throw std::runtime_error("spc_base::get_spectra: unknown type of name");
    }
    if (this->find(name) == this->end()) {
      throw std::runtime_error("spc_base::get_spectra: spectra with name " + this->get_name(name) + " does not exist");
    }
    return this->at(name);
  }

  /*!
   * @brief convience function for two strings; if we provide two strings, we assume that we want a cross spectrum, we can not add remote or emap
   */
  std::shared_ptr<std::vector<T>> get_spectra(const std::string &name_in, const std::string &name_in2) const {
    auto name = std::pair<std::string, std::string>(name_in, name_in2);
    return this->get_spectra(name);
  }

  // 2) as std::vector - that is a copy! **************
  /*!
   * @brief Retrieves a single spectrum from the collection as a vector.
   * @param name The name of the spectrum to retrieve.
   * @return A vector containing the spectrum data.
   * @throws std::runtime_error if the spectrum with the given name does not exist.
   */
  template <typename S>
  std::vector<T> get_spectra_vec(const S &name_out, const bool is_remote = false, const bool is_emap = false) const {
    return *this->get_spectra(name_out, is_remote, is_emap);
  }

  /*!
   * @brief convience function for two strings; if we provide two strings, we assume that we want a cross spectrum, we can not add remote or emap
   */
  std::vector<T> get_spectra_vec(const std::string &name_in, const std::string &name_in2) const {
    auto name = std::pair<std::string, std::string>(name_in, name_in2);
    return this->get_spectra_vec(name);
  }
  // ******************************************************  D E L E T I N G   S P E C T R A  ****************************************************************
  /**
   * @brief Deletes a single spectrum from the collection.
   * @param name The name of the spectrum to delete.
   * @throws std::runtime_error if the spectrum with the given name does not exist.
   */
  template <typename S>
  void delete_spectra(const S &name_in) {
    std::shared_lock lock(spc_lock);
    std::pair<std::string, std::string> name;
    if constexpr (std::is_same_v<S, std::pair<std::string, std::string>>) {
      name = name_in;
    } else if constexpr (std::is_same_v<S, std::string>) {
      name = std::pair<std::string, std::string>(name_in, "");
    } else {
      throw std::runtime_error("spc_base::delete: unknown type of name");
    }
    if (this->find(name) == this->end()) {
      throw std::runtime_error("spc_base::delete_spectra: spectra with name " + this->get_name(name) + " does not exist");
    }
    this->erase(name);
  }

  /*!
   * @brief convience function for two strings
   */
  void delete_spectra(const std::string &name_in, const std::string &name_in2) {
    auto name = std::pair<std::string, std::string>(name_in, name_in2);
    this->delete_spectra(name);
  }

  // ****************************************************** O T H E R functions   S P E C T R A  ****************************************************************

  T get_stack_no(const std::pair<std::string, std::string> &name, const size_t &stack_no) const {
    // check that this container is type of T = std::vector
    // check that this container is type of std::vector
    static_assert(std::is_same_v<T, std::vector<typename T::value_type>>, "T must be std::vector");
    if (this->find(name) == this->end()) {
      throw std::runtime_error("spc_base::get_slice_no: spectra with name " + this->get_name(name) + " does not exist");
    }
    return this->at(name)->at(stack_no);
  }

  void set_bw(const double bw) { this->bw = bw; }
  double get_bw() const { return this->bw; }
  bool is_auto_spc(const std::pair<std::string, std::string> &name) const {
    return (name.first == name.second);
  }
  bool is_cross_spc(const std::pair<std::string, std::string> &name) const {
    return (name.first != name.second);
  }
  bool is_single_spc(const std::pair<std::string, std::string> &name) const {
    return (name.second.empty());
  }
  bool treat_as_auto_spc(const std::pair<std::string, std::string> &name) const {
    // remove a leading 'R' from the name if it is there
    std::string my_name1 = name.first;
    if (name.first[0] == 'R') {
      my_name1 = name.first.substr(1);
    }
    std::string my_name2 = name.second;
    if (name.second[0] == 'R') {
      my_name2 = name.second.substr(1);
    }
    return (my_name1 == my_name2);
  }

  void rename_to_auto(const std::string &single) {
    std::shared_lock lock(spc_lock);
    auto name = std::pair<std::string, std::string>(single, "");
    if (this->find(name) == this->end()) {
      throw std::runtime_error("spc_base::rename_to_auto: spectra with name " + this->get_name(name) + " does not exist");
    }
    auto new_name = std::pair<std::string, std::string>(single, single);
    if (this->find(new_name) != this->end()) {
      throw std::runtime_error("spc_base::rename_to_auto: spectra with name " + this->get_name(new_name) + " already exists");
    }
    this->emplace(std::pair<std::string, std::string>(single, single), this->at(name));
    this->erase(name);
  }

  void rename(const std::string &old_name_1, const std::string &old_name_2, const std::string &new_name_1, const std::string &new_name_2) {
    std::shared_lock lock(spc_lock);
    auto old_name = std::pair<std::string, std::string>(old_name_1, old_name_2);
    if (this->find(old_name) == this->end()) {
      throw std::runtime_error("spc_base::rename: spectra with name " + this->get_name(old_name) + " does not exist");
    }
    auto new_name = std::pair<std::string, std::string>(new_name_1, new_name_2);
    if (this->find(new_name) != this->end()) {
      throw std::runtime_error("spc_base::rename: spectra with name " + this->get_name(new_name) + " already exists");
    }
    this->emplace(new_name, this->at(old_name));
    this->erase(old_name);
  }

private:
  double bw = 0.0; // bandwidth - that is for the complete collection !!
  mutable std::shared_mutex spc_lock;
  std::string get_name(const std::pair<std::string, std::string> &name) const {
    return name.first + name.second;
  }
}; // class spc_base

#endif // SPC_BASE_H