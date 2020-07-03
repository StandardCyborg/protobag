#include <iostream>

#include <protobag/Protobag.hpp>
#include <protobag/Utils/PBUtils.hpp>

#include "MyMessages.pb.h"

using namespace my_messages;

int main() {
  protobag::Protobag bag("example_bag.zip");
  
  auto maybeWriter = bag.StartWriteSession();
  if (!maybeWriter.IsOk() {
    std::cerr << "Failed to start writing: " << maybeWriter.error << std::endl;
    return -1;
  }

  auto &writer = *maybeWriter.value;

  {
    DinoHunter max;
    max.set_first_name("max");
    max.set_id(1);
    auto *dino = max.add_dinos();
    dino->set_name("nibbles");
    dino->set_type(DinoHunter_DinoType::DinoHunter_DinoType_PEOPLEEATINGSAURUS);

    auto status = writer->WriteEntry(
      protobag::Entry::Create(
        "hunters/max",
        max));
    if (!status.IsOk()) {
      std::cerr << "Failed to write Max" << std::endl;
      return -2;
    }

    std::cout << "Wrote Max: " << protobag::PBToString(max) << std::endl;
  }

  {
    DinoHunter lara;
    lara.set_first_name("Lara");
    lara.set_id(2);
    auto *dino1 = lara.add_dinos();
    dino1->set_name("bites");
    dino1->set_type(DinoHunter_DinoType::DinoHunter_DinoType_PEOPLEEATINGSAURUS);

    auto *dino2 = lara.add_dinos();
    dino2->set_name("stinky");
    dino2->set_type(DinoHunter_DinoType::DinoHunter_DinoType_VEGGIESAURUS);

    auto status = writer->WriteEntry(
      protobag::Entry::Create(
        "hunters/Lara",
        lara));
    
    if (!status.IsOk()) {
      std::cerr << "Failed to write Lara" << std::endl;
      return -3;
    }

    std::cout << "Wrote Lara: " << protobag::PBToString(lara) << std::endl;
  }

  {
    // A Chase!
    for (int t = 0; t < 10; t++) {
      Position lara_pos; lara_pos.set_x(t); lara_pos.set_y(t + 1);
      Position toofz_pos; toofz_pos.set_x(t + 2); toofz_pos.set_y(t + 3);

      auto status = writer->WriteEntry(
        protobag::Entry::CreateStamped(
          "positions/lara",
          t, 0,
          lara_pos));
      if (!status.IsOk()) { std::cerr << "Chase write failed\n"; return -t; }

      auto status = writer->WriteEntry(
        protobag::Entry::CreateStamped(
          "positions/toofz",
          t, 0,
          toofz_pos));
      if (!status.IsOk()) { std::cerr << "Chase write failed\n"; return -t; }
    }
  }

  writer->Close();
  std::cout << "Wrote to: " << bag.path << std::endl;
  return 0;
}