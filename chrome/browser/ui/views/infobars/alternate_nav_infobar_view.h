// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_INFOBARS_ALTERNATE_NAV_INFOBAR_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_INFOBARS_ALTERNATE_NAV_INFOBAR_VIEW_H_

#include "base/memory/scoped_ptr.h"
#include "chrome/browser/ui/views/infobars/infobar_view.h"
#include "ui/views/controls/link_listener.h"

class AlternateNavInfoBarDelegate;

// An infobar that shows a string with an embedded link.
class AlternateNavInfoBarView : public InfoBarView,
                                public views::LinkListener {
 public:
  explicit AlternateNavInfoBarView(
      scoped_ptr<AlternateNavInfoBarDelegate> delegate);

 private:
  virtual ~AlternateNavInfoBarView();

  // Treating |labels| as pieces of one continuous string, elides to fit
  // |available_width| so as to guarantee that a trailing ellipsis is always
  // displayed when the string is elided and there is at least room to display
  // a lone ellipsis.
  //
  // NOTE: This may modify the text of any/all of the labels, so reset their
  // texts when the available width changes before calling this again.
  static void ElideLabels(Labels* labels, int available_width);

  // InfoBarView:
  virtual void Layout() OVERRIDE;
  virtual void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) OVERRIDE;
  virtual int ContentMinimumWidth() OVERRIDE;

  // views::LinkListener:
  virtual void LinkClicked(views::Link* source, int event_flags) OVERRIDE;

  AlternateNavInfoBarDelegate* GetDelegate();

  base::string16 label_1_text_;
  base::string16 link_text_;
  base::string16 label_2_text_;

  views::Label* label_1_;
  views::Link* link_;
  views::Label* label_2_;

  DISALLOW_COPY_AND_ASSIGN(AlternateNavInfoBarView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_INFOBARS_ALTERNATE_NAV_INFOBAR_VIEW_H_
