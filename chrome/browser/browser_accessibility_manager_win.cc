// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_accessibility_manager_win.h"

#include "chrome/browser/browser_accessibility_win.h"
#include "chrome/browser/renderer_host/render_process_host.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/common/render_messages.h"

using webkit_glue::WebAccessibility;

// Factory method to create an instance of BrowserAccessibility
BrowserAccessibility* BrowserAccessibilityFactory::Create() {
  CComObject<BrowserAccessibility>* instance;
  HRESULT hr = CComObject<BrowserAccessibility>::CreateInstance(&instance);
  DCHECK(SUCCEEDED(hr));
  return instance->NewReference();
}

// static
// Start child IDs at -1 and decrement each time, because clients use
// child IDs of 1, 2, 3, ... to access the children of an object by
// index, so we use negative IDs to clearly distinguish between indices
// and unique IDs.
LONG BrowserAccessibilityManager::next_child_id_ = -1;

BrowserAccessibilityManager::BrowserAccessibilityManager(
    HWND parent_hwnd,
    const webkit_glue::WebAccessibility& src,
    BrowserAccessibilityDelegate* delegate,
    BrowserAccessibilityFactory* factory)
    : parent_hwnd_(parent_hwnd),
      delegate_(delegate),
      factory_(factory),
      focus_(NULL) {
  HRESULT hr = ::CreateStdAccessibleObject(
      parent_hwnd_, OBJID_WINDOW, IID_IAccessible,
      reinterpret_cast<void **>(&window_iaccessible_));
  DCHECK(SUCCEEDED(hr));
  root_ = CreateAccessibilityTree(NULL, src, 0);
  if (!focus_)
    focus_ = root_;
}

BrowserAccessibilityManager::~BrowserAccessibilityManager() {
  // Clients could still hold references to some nodes of the tree, so
  // calling Inactivate will make sure that as many nodes as possible are
  // released now, and remaining nodes are marked as inactive so that
  // calls to any methods on them will return E_FAIL;
  root_->InactivateTree();
  root_->Release();
}

BrowserAccessibility* BrowserAccessibilityManager::GetRoot() {
  return root_;
}

BrowserAccessibility* BrowserAccessibilityManager::GetFromChildID(
    LONG child_id) {
  base::hash_map<LONG, BrowserAccessibility*>::iterator iter =
      child_id_map_.find(child_id);
  if (iter != child_id_map_.end()) {
    return iter->second;
  } else {
    return NULL;
  }
}

IAccessible* BrowserAccessibilityManager::GetParentWindowIAccessible() {
  return window_iaccessible_;
}

HWND BrowserAccessibilityManager::GetParentHWND() {
  return parent_hwnd_;
}

BrowserAccessibility* BrowserAccessibilityManager::GetFocus(
    BrowserAccessibility* root) {
  if (focus_ && (!root || focus_->IsDescendantOf(root)))
    return focus_;

  return NULL;
}

void BrowserAccessibilityManager::SetFocus(const BrowserAccessibility& node) {
  if (delegate_)
    delegate_->SetAccessibilityFocus(node.renderer_id());
}

void BrowserAccessibilityManager::DoDefaultAction(
    const BrowserAccessibility& node) {
  if (delegate_)
    delegate_->AccessibilityDoDefaultAction(node.renderer_id());
}

void BrowserAccessibilityManager::OnAccessibilityFocusChange(int renderer_id) {
  base::hash_map<int, LONG>::iterator iter =
      renderer_id_to_child_id_map_.find(renderer_id);
  if (iter == renderer_id_to_child_id_map_.end())
    return;

  LONG child_id = iter->second;
  base::hash_map<LONG, BrowserAccessibility*>::iterator uniq_iter =
    child_id_map_.find(child_id);
  if (uniq_iter != child_id_map_.end())
    focus_ = uniq_iter->second;
  ::NotifyWinEvent(EVENT_OBJECT_FOCUS, parent_hwnd_, OBJID_CLIENT, child_id);
}

void BrowserAccessibilityManager::OnAccessibilityObjectStateChange(
    int renderer_id) {
  base::hash_map<int, LONG>::iterator iter =
      renderer_id_to_child_id_map_.find(renderer_id);
  if (iter == renderer_id_to_child_id_map_.end())
    return;

  LONG child_id = iter->second;
  ::NotifyWinEvent(EVENT_OBJECT_FOCUS, parent_hwnd_, OBJID_CLIENT, child_id);
}

BrowserAccessibility* BrowserAccessibilityManager::CreateAccessibilityTree(
    BrowserAccessibility* parent,
    const webkit_glue::WebAccessibility& src,
    int index_in_parent) {
  BrowserAccessibility* instance = factory_->Create();

  // Get the next child ID, and wrap around when we get near the end
  // of a 32-bit integer range. It's okay to wrap around; we just want
  // to avoid it as long as possible because clients may cache the ID of
  // an object for a while to determine if they've seen it before.
  LONG child_id = next_child_id_;
  next_child_id_--;
  if (next_child_id_ == -2000000000)
    next_child_id_ = -1;

  instance->Initialize(this, parent, child_id, index_in_parent, src);
  child_id_map_[child_id] = instance;
  renderer_id_to_child_id_map_[src.id] = child_id;
  if ((src.state >> WebAccessibility::STATE_FOCUSED) & 1)
    focus_ = instance;
  for (int i = 0; i < static_cast<int>(src.children.size()); ++i) {
    BrowserAccessibility* child = CreateAccessibilityTree(
        instance, src.children[i], i);
    instance->AddChild(child);
  }

  return instance;
}
