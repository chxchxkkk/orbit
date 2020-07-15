#include "LiveFunctions.h"

#include <utility>

#include "TimeGraph.h"

std::pair<uint64_t, uint64_t> ComputeMinMaxTime(
    const std::vector<TextBox*> text_boxes) {
  uint64_t min_time = std::numeric_limits<uint64_t>::max();
  uint64_t max_time = std::numeric_limits<uint64_t>::min();
  for (size_t k = 0; k < text_boxes.size(); ++k) {
    min_time = std::min(min_time, text_boxes[k]->GetTimer().m_Start);
    // For multiple events, the visualization is such that the duration
    // from the first to the start of the last event is shown. Therefore,
    // we use m_Start below.
    max_time = std::max(max_time, text_boxes[k]->GetTimer().m_Start);
  }
  if (min_time == max_time) {
    // If we only have a single box or all events fall onto the same start,
    // we want to zoom such that at least one event is visible,
    // so we take the time from start to end of the first event.
    const Timer& timer = text_boxes[0]->GetTimer();
    max_time = timer.m_End;
  }
  return std::make_pair(min_time, max_time);
}

void LiveFunctions::Move() {
  std::vector<TextBox*> text_boxes;
  for (auto it : current_textboxes_) {
    text_boxes.push_back(it.second);
  }
  if (!text_boxes.empty()) {
    auto min_max = ComputeMinMaxTime(text_boxes);
    GCurrentTimeGraph->Zoom(min_max.first, min_max.second);
  } else {
    GCurrentTimeGraph->ZoomAll();
  }
  GCurrentTimeGraph->SetCurrentTextBoxes(text_boxes);
}

bool LiveFunctions::OnAllNextButton() {
  absl::flat_hash_map<uint64_t, TextBox*> next_boxes;
  for (auto it : function_iterators_) {
    Function* function = it.second;
    TextBox* current_box = current_textboxes_.find(it.first)->second;
    TextBox* box = live_functions_data_view_.FindNext(
        *function, current_box->GetTimer().m_End);
    if (box == nullptr) {
      return false;
    }
    next_boxes.insert(std::make_pair(it.first, box));
  }
  // We only want to commit to the new boxes when all boxes can be moved.
  current_textboxes_ = next_boxes;
  Move();
  return true;
}

bool LiveFunctions::OnAllPreviousButton() {
  absl::flat_hash_map<uint64_t, TextBox*> next_boxes;
  for (auto it : function_iterators_) {
    Function* function = it.second;
    TextBox* current_box = current_textboxes_.find(it.first)->second;
    TextBox* box = live_functions_data_view_.FindPrevious(
        *function, current_box->GetTimer().m_End);
    if (box == nullptr) {
      return false;
    }
    next_boxes.insert(std::make_pair(it.first, box));
  }

  // We only want to commit to the new boxes when all boxes can be moved.
  current_textboxes_ = next_boxes;
  Move();
  return true;
}

void LiveFunctions::OnNextButton(uint64_t id) {
  TextBox* text_box = live_functions_data_view_.FindNext(
      *(function_iterators_[id]), current_textboxes_[id]->GetTimer().m_End);
  // If text_box is nullptr, then we have reached the right end of the timeline.
  if (text_box != nullptr) {
    current_textboxes_[id] = text_box;
    Move();
  }
}
void LiveFunctions::OnPreviousButton(uint64_t id) {
  TextBox* text_box = live_functions_data_view_.FindPrevious(
      *(function_iterators_[id]), current_textboxes_[id]->GetTimer().m_Start);
  // If text_box is nullptr, then we have reached the left end of the timeline.
  if (text_box != nullptr) {
    current_textboxes_[id] = text_box;
    Move();
  }
}

void LiveFunctions::OnDeleteButton(uint64_t id) {
  current_textboxes_.erase(id);
  function_iterators_.erase(id);
  Move();
}

void LiveFunctions::AddIterator(Function* function, TextBox* current_textbox) {
  uint64_t id = next_id;
  ++next_id;

  function_iterators_.insert(std::make_pair(id, function));
  current_textboxes_.insert(std::make_pair(id, current_textbox));
  if (add_iterator_callback_) {
    add_iterator_callback_(id, function);
  }
  Move();
}