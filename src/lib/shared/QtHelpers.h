//
// Created by jglanz on 8/28/22.
//

#pragma once


template<typename Ptr> void qQuitPointer(Ptr ptr) {
  ptr->quit();
}

template<typename Ptr> void qExitPointer(Ptr ptr) {
  ptr->exit();
}
