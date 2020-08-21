  
/*
Copyright 2020 Standard Cyborg
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <iostream>

#include <protobag/Protobag.hpp>
#include <protobag/Utils/PBUtils.hpp>

#include "MyMessages.pb.h"

using namespace my_messages;

int main() {
  protobag::Protobag bag("example_bag.zip");

  #define PRINT_AND_EXIT(msg) do { \
    std::cerr << msg << std::endl; \
    return -1; \
  } while(0)

  auto maybeWriter = bag.StartWriteSession();
  if (!maybeWriter.IsOk()) {
    PRINT_AND_EXIT("Failed to start writing: " << maybeWriter.error);
  }

  auto &writer = *maybeWriter.value;


  ///
  /// Write some standalone entries
  ///
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
      PRINT_AND_EXIT("Failed to write Max: " << status.error);
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
      PRINT_AND_EXIT("Failed to write Lara: " << status.error);
    }

    std::cout << "Wrote Lara: " << protobag::PBToString(lara) << std::endl;
  }


  ///
  /// Use time series data API
  ///
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
      if (!status.IsOk()) {
        PRINT_AND_EXIT(
          "Chase failed to write at " << t << ": " << status.error);
      }

      status = writer->WriteEntry(
        protobag::Entry::CreateStamped(
          "positions/toofz",
          t, 0,
          toofz_pos));
      if (!status.IsOk()) {
        PRINT_AND_EXIT(
          "Chase failed to write at " << t << ": " << status.error);
      }
    }
  }


  ///
  /// Use Raw API
  ///
  {
    std::string raw_data = "i am a raw string";
    auto status = writer->WriteEntry(
        protobag::Entry::CreateRawFromBytes(
          "raw_data",
          std::move(raw_data)));
      if (!status.IsOk()) {
        PRINT_AND_EXIT(
          "Raw write failed: " << status.error);
      }
  }

  writer->Close();
  std::cout << "Wrote to: " << bag.path << std::endl;
  return 0;
}