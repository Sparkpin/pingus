//  $Id: inverted_axis.cxx,v 1.2 2002/07/05 10:06:20 torangan Exp $
// 
//  Pingus - A free Lemmings clone
//  Copyright (C) 2000 Ingo Ruhnke <grumbel@gmx.de>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
// 
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "inverted_axis.hxx"

namespace Input
{
  InvertedAxis::InvertedAxis(Axis* axis_) : axis(axis_)
  {
    angle = (static_cast<int>(axis->get_angle()) + 180) % 360;
  }

  float
  InvertedAxis::get_pos ()
  {
    return axis->get_pos();
  }

  float
  InvertedAxis::get_angle ()
  {
    return angle;
  }
  
  void
  InvertedAxis::update(float delta)
  {
    axis->update(delta);
  }
}

/* EOF */
