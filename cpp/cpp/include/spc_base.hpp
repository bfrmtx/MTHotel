#ifndef SPC_BASE_HPP
#define SPC_BASE_HPP
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

#include "cal_base.hpp"
#include "channel.hpp"
#include "mt_base.hpp"

/**
 * @brief A class template for managing spectra. T is at least a std::vector<T> or std::vector<std::vector<T>>.
 *
 * This class is derived from std::map and provides functionality for adding, moving, retrieving, and deleting spectra.
 * Each  spectrum is associated with a name and stored as a shared pointer to a vector or a vector of vectors.
 * The class also allows setting and getting the bandwidth of the spectra.
 * Do not use channel = channel; we don't want counters on channel objects here; later in main it is desired to delete the shared pointers
 * The channel object CONTAINS THE FREQUENCIES! <br>
 * ----> vector: that is e.g. a stacked result, and in case of coherence of type double <br>
 * ----> vector of vectors: that is e.g. unstacked (first vector) of complex spectra (second vector) <br>
 * the channel wich we created should contain the frequencies, same size as the spectra (second vector) <br>
 * the <b>internal ch_map</b> shall contain the channel objects, which are used to create the spectra! Must be a pair map object!
 * a <Hx,> is mostly the vector of vector of complex raw spectra, which you want to work on
 * a <Hx,Hx> or <Hx, Hy>  is <br>
 * a) a vector of vectors of complex spectra, which you want to work on (where <Hx, Hx> is auto in should be real)
 * a1) if we have <Hx,Hy> of <Hx,Hx> - we always have two channel objects! even though we have ONE data vector.
 * b) a vector of (mostly real) spectra containing the stacked spectra, calculated from the vector of vectors of complex spectra <br>
 * c) a vector of coherence or noise which are definition real
 *
 * @tparam T The type of the elements in the spectra vector, like double or std::complex<double> OR <std::vector<std::complex<double>>>, <std::vector<std::double>>
 */
template <typename T>
struct is_std_vector : std::false_type {};
template <typename U, typename Alloc>
struct is_std_vector<std::vector<U, Alloc>> : std::true_type {};

template <typename T>
class spc_base : public std::map<std::pair<std::string, std::string>, std::shared_ptr<std::vector<T>>> {
public:
  spc_base() = default;
  ~spc_base() = default;

  // ******************************************************  M O V I N G  / A D D I N G   S P E C T R A  ****************************************************************
  // 1) ********* for shared pointers ************
  /**
   * @brief Moves a single spectrum to the collection, e.g. from a channel object
   * @param name_in The name of the spectrum. If name is a single string, the second name is empty so Hx -> <Hx, > and <Hx, Hy> -> <Hx, Hy>
   * @param spectra The vector containing the spectral data. It will be **** MOVED **** into the map. if spectra is  N U L L P T R  , an empty vector will be created <br>
   *   When we set the single spectra, like form a channel object, it is <b> STD::VECTOR<STD::VECTOR<STD::COMPLEX<DOUBLE>>> </b>
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
      throw std::runtime_error("spc_base::add_spectra: unknown type of name, should be std::pair<std::string, std::string>   or std::string");
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
   * @details a channel ONLY contains his own spectrum, so not <Ex, Ey> etc.
   * @param chan the channel object can be set to be remote or emap; this will be considered
   */
  void move_spectra(std::shared_ptr<channel> chan) {
    std::string name_in = chan->channel_type;
    if (is_E(name_in) && chan->is_emap && !chan->is_remote) {
      name_in = "E" + name_in;
    }
    if (!chan->is_emap && chan->is_remote) {
      name_in = "R" + name_in;
    }
    auto name = std::pair<std::string, std::string>(name_in, "");
    this->add_spectra(name, chan->spc, chan->bw, true);
    this->add_channel(name, chan, nullptr); // this is in main done after running the fft; so we should have the frequencies in channel
    this->spc_t = get_spc_type_from_name(name);
  }

  /*!
   * @brief add a channel object; this will be newly created, not moved or copied
   * @param name
   * @param chan1
   * @param chan2
   * @param overwrite at the beginning the channel object may have no frequencies, so we can overwrite it later
   */
  void add_channel(const std::pair<std::string, std::string> name, const std::shared_ptr<channel> chan1, const std::shared_ptr<channel> chan2 = nullptr, const bool overwrite = false) {

    if (overwrite) {
      if (this->ch_map.find(name) != this->ch_map.end()) {
        this->ch_map.erase(name);
      } else {
        throw std::runtime_error("spc_base::add_channel: channel with name " + this->get_name(name) + " does not exist, can' update / overwrite");
      }
    }

    if (this->ch_map.find(name) != this->ch_map.end()) {
      throw std::runtime_error("spc_base::add_channel: channel with name " + this->get_name(name) + " already exists");
    }
    if (chan1 == nullptr) {
      throw std::runtime_error("spc_base::add_channel: channel 1 is nullptr");
    }
    // if we have tow channels both appear as <Hx, > and <Hy, >, so we need to add them both
    this->ch_map.emplace(name, std::make_pair(std::make_shared<channel>(chan1), std::make_shared<channel>(chan2)));
  }

  /*!
   * @brief prepares EMPTY spectra for auto and cross spectra; we need to know the channel names
   * @param name the name of the spectra like HxHx of HxHy or ExHy
   * @param chan1 the first channel
   * @param chan2 the second channel, we always need two channels for auto or cross spectra!
   * @note This is mostly in main where you decide what to calculate.
   * @note for coherence we later need a a stacked object (for the cross channels) AND a unstacked for the raw, unstacked spectra
   */
  void prepare_ac_cross_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> chan_pair) {
    auto chan1 = chan_pair.first;
    auto chan2 = chan_pair.second;
    // check for null pointers
    if (chan1 == nullptr) {
      throw std::runtime_error("spc_base::prepare_ac_cross_spectra: channel 1 is nullptr");
    }
    if (chan2 == nullptr) {
      throw std::runtime_error("spc_base::prepare_ac_cross_spectra: channel 2 is nullptr");
    }
    std::string name_in1 = chan1->channel_type;
    std::string name_in2 = chan2->channel_type;
    if (is_E(name_in1) && chan1->is_emap && !chan1->is_remote) {
      name_in1 = "E" + name_in1;
    }
    if (!chan1->is_emap && chan1->is_remote) {
      name_in1 = "R" + name_in1;
    }
    if (is_E(name_in2) && chan2->is_emap && !chan2->is_remote) {
      name_in2 = "E" + name_in2;
    }
    if (!chan2->is_emap && chan2->is_remote) {
      name_in2 = "R" + name_in2;
    }
    std::pair<std::string, std::string> name(name_in1, name_in2);
    this->emplace(name, std::make_shared<std::vector<T>>());
    this->add_channel(name, chan1, chan2);
    this->spc_t = get_spc_type_from_name(name);
  }

  // we add two diffent channels, like Hx and Hy, or Ex and Ey; make sure that this base class is double, not complex
  // coherence is a real value.
  void prepare_cross_coherence(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> chan_pair) {
    auto chan1 = chan_pair.first;
    auto chan2 = chan_pair.second;
    std::string name_in1 = chan1->channel_type;
    std::string name_in2 = chan2->channel_type;
    if (is_E(name_in1) && chan1->is_emap && !chan1->is_remote) {
      name_in1 = "E" + name_in1;
    }
    if (!chan1->is_emap && chan1->is_remote) {
      name_in1 = "R" + name_in1;
    }
    if (is_E(name_in2) && chan2->is_emap && !chan2->is_remote) {
      name_in2 = "E" + name_in2;
    }
    if (!chan2->is_emap && chan2->is_remote) {
      name_in2 = "R" + name_in2;
    }
    std::pair<std::string, std::string> name(name_in1, name_in2);
    this->emplace(name, std::make_shared<std::vector<T>>());
    this->spc_t = get_spc_type_from_name(name);
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

  std::vector<std::pair<std::string, std::string>> get_spectra_names() const {
    std::shared_lock lock(spc_lock);
    std::vector<std::pair<std::string, std::string>> names;
    for (const auto &spc : *this) {
      names.push_back(spc.first);
    }
    return names;
  }
  std::map<std::pair<std::string, std::string>, std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> get_channel_map() const {
    return this->ch_map;
  }

  // for prepare_cross_coherency we need a permutation of single spectra, so we take <Hx, >, <Hy, >, <Hz, > and so on and return <Hx, Hy>, <Hx, Hz> and so on
  std::vector<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> get_channel_pairs_permutation() const {
    // fist we check that we have single spectra, so all second names are empty
    for (const auto &ch : this->ch_map) {
      if (ch.first.second != "") {
        throw std::runtime_error("spc_base::get_channel_pairs_permutation: we are maybe not dealing with SINGLE spectra");
      }
    }

    std::vector<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> chan_pairs;
    for (size_t i = 0; i < this->ch_map.size() - 1; i++) {
      for (size_t j = i + 1; j < this->ch_map.size(); j++) {
        auto it1 = this->ch_map.begin();
        std::advance(it1, i);
        auto it2 = this->ch_map.begin();
        std::advance(it2, j);
        chan_pairs.push_back(std::make_pair(it1->second.first, it2->second.first));
      }
    }
    return chan_pairs;
  }

  std::vector<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> get_channel_pairs_cross() const {
    std::vector<std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> chan_pairs;
    for (const auto &ch : this->ch_map) {
      if ((ch.first.second != "") && (ch.first.first != ch.first.second)) { // we have a cross spectrum if additional channel is not empty AND different from the first
        chan_pairs.push_back(ch.second);
      }
    }
    return chan_pairs;
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
    this->ch_map.erase(name);
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
    // Ensure T is a vector at compile time if possible, otherwise rely on documentation and runtime checks.
    if constexpr (!std::is_same_v<T, std::vector<typename T::value_type>>) {
      throw std::runtime_error("spc_base::get_stack_no: T must be std::vector");
    }
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
  /*!
   * @brief get the size of the map and the size of the vector in the map; throw an exception if the data type is a vector of vectors
   * @return a pair with the size of the map and the size of the vector in the map
   */
  std::pair<size_t, size_t> get_size_stacked() const {
    std::shared_lock lock(spc_lock);
    if (this->empty()) {
      return std::make_pair(0, 0);
    }
    // check if T is a double or std::complex<double> or int
    using T_no_cvref = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (!(std::is_same_v<T_no_cvref, double> ||
                    std::is_same_v<T_no_cvref, std::complex<double>> ||
                    std::is_same_v<T_no_cvref, int>)) {
      throw std::runtime_error("spc_base::get_size_stacked: T must be double, std::complex<double>, or int");
    }
    // if constexpr (!is_std_vector<T>::value) {
    //   throw std::runtime_error("spc_base::get_size_stacked: T must be std::vector<U>");
    // }
    // std::map<std::pair<std::string, std::string>, std::shared_ptr<std::vector<T>>>
    // - `this->size()` returns the number of elements in the map.
    // - `this->begin()->first` gets the key of the first element in the map.
    // - `like key <Hx,Hx>`
    // - `this->at(this->begin()->first)` retrieves the `shared_ptr<std::vector<T>>` associated with that key.
    // - `the at function returns the value of the map at the given key, which is a shared pointer to a vector of T`.
    // - `->size()` gets the size of the vector pointed to by that `shared_ptr`.
    return std::make_pair(this->size(), this->at(this->begin()->first)->size());
  }
  /*!
   * @brief get the size of the map and the size of the vector in the map; throw an exception if the data type is NOT a vector of vectors
   * @param nstacks will be set to the number of stacks, that is the size of the outer vector
   * @return a pair with the size of the map and the size of the inner vector in the map, aka frequency vector
   * @throws std::runtime_error if the data type is not a vector of vectors
   */
  std::pair<size_t, size_t> get_size_stacked(size_t &nstacks) const {
    std::shared_lock lock(spc_lock);
    if (this->empty()) {
      nstacks = 0;
      return std::make_pair(0, 0);
    }
    // map is of type std::map<std::pair<std::string, std::string>, std::shared_ptr<std::vector<std::vector<T>>>>
    // example: <Hx,Hx> , vector(60, vector<std::complex<double>>(1024))
    // so <Hx,Hx> has 60 times a vector of std::complex<double> with 1024 elements
    // so nstacks is the size of the outer vector, that is the number of stacks, that is 60
    // and the inner vector is the data vector, that is 1024 (corresponding to the frequencies we have in the spectra)
    // check that T is a vector of vectors, that is std::vector<std::vector<T>>
    // access (for example <Hx, Hx>): get map value by key hx_hx_vector = this->at(Hx, Hx)
    // hx_hx_vector.at(0) is a vector of 1024 elements, containing std::complex<double> data
    //
    // example code to get the inner vector for Hx,Hx:
    // we have a function mean(const std::vector<std::complex<double>> &v) which calculates the mean of a vector of doubles
    // std::pair<std::string, std::string> key = std::make_pair("Hx", "Hx");
    // std::shared_ptr<std::vector<std::vector<double>>> hx_hx_vector = this->at(key);
    // for (const auto &inner_vector : *hx_hx_vector) {
    //   double mean_value = mean(inner_vector);
    // }
    //
    // with the help of the shared pointer we can easily access the vector with the "=" operator, so no copy is made
    // outside we can access the inner vector with the "->" operator, so no copy is made
    // or pass the inner vector to a function with (example first one)
    // std::vector<std::complex<double>> inner_vector = hx_hx_vector->at(0);
    // or using the [] operator
    // std::vector<std::complex<double>> inner_vector = (*hx_hx_vector)[0];
    // or using the comfortable way in an auto loop
    // for (const auto &inner_vector : *hx_hx_vector) or if you modify the inner vector
    // for (auto &inner_vector : *hx_hx_vector) // this is a reference to the inner vector, so you can modify it
    // so we can access the inner vector with the "->" operator, so no copy is made
    // or pass the inner vector to a function with (example first one)
    // std::vector<std::complex<double>> inner_vector = hx_hx_vector->at(0);
    // or using the [] operator
    // std::vector<std::complex<double>> inner_vector = (*hx_hx_vector)[0];
    // hence that a shared pointer needs to be dereferenced with the "*" operator to access the vector of vectors inside the shared pointer

    // we use a static_assert to check that T is a vector of vectors at compile time
    // check if the first element of the map is a vector of vectors
    if constexpr (!std::is_same_v<T, std::vector<typename T::value_type>>) {
      throw std::runtime_error("spc_base::get_size_stacked: T must be std::vector<std::vector<T>>");
    }
    // std::map<std::pair<std::string, std::string>, std::shared_ptr<std::vector<std::vector<T>>>>
    // - `this->size()` returns the number of elements in the map.
    // - `this->begin()->first` gets the key of the first element in the map.
    // - `like key <Hx,Hx>`
    // - `this->at(this->begin()->first)` retrieves the `shared_ptr<std::vector<std::vector<T>>>` associated with that key.
    // - `the at function returns the value of the map at the given key, which is a shared pointer to a vector of vectors of T`.
    // - `->size()` gets the size of the outer vector pointed to by that `shared_ptr`.
    nstacks = this->at(this->begin()->first)->size();
    if (nstacks == 0) {
      throw std::runtime_error("spc_base::get_size_stacked: outer vector is empty, cannot determine size");
    }
    size_t inner_size = this->at(this->begin()->first)->at(0).size(); // size of the inner vector, that is the frequency vector

    if (inner_size == 0) {
      throw std::runtime_error("spc_base::get_size_stacked: outer vector is empty, cannot determine size");
    }
    // return the size of the map and the size of the inner vector
    // that is the size of the frequency vector
    // std::make_pair(this->size(), inner_size) returns a pair with the size of the map and the size of the inner vector
    // that is the size of the frequency vector

    return std::make_pair(this->size(), inner_size);
  }

  void info() {
    // print out all names
    // std::map<std::pair<std::string, std::string>, std::shared_ptr<std::vector<T>>>
    // so all names are in the first part of the pair of this map

    std::shared_lock lock(spc_lock);
    for (const auto &spc : *this) {
      std::cout << "spc base::spectrum: " << spc.first.first;
      if (!spc.first.second.empty())
        std::cout << ", " << spc.first.second;
      std::cout << std::endl;
      // now we can print out the size of the vector, sample rate, rl, etc.
    }
    for (const auto &ch : this->ch_map) {
      std::cout << "spc base::channel: " << ch.first.first;
      if (!ch.first.second.empty())
        std::cout << ", " << ch.first.second;
      if (ch.second.first) {
        std::cout << "  " << ch.second.first->get_sample_rate() << " Hz" << std::endl;
      }
    }
  }

private:
  double bw = 0.0; // bandwidth - that is for the complete collection !!
  mutable std::shared_mutex spc_lock;
  spc_type spc_t = spc_type::null; // we keep track of the type of the spectra

  // we keep a deep copy of the channel data
  // Map from spectrum name to a pair of channels (for cross/auto spectra)
  std::map<std::pair<std::string, std::string>, std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>>> ch_map;

  std::string
  get_name(const std::pair<std::string, std::string> &name) const {
    return name.first + name.second;
  }
}; // class spc_base

#endif // SPC_BASE_HPP