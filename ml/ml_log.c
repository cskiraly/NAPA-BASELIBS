/*
 * Copyright (c) 2009 NEC Europe Ltd. All Rights Reserved.
 * Authors: Kristian Beckers  <beckers@nw.neclab.eu>
 *          Sebastian Kiesel  <kiesel@nw.neclab.eu>
 *
 * This file is part of the Messaging Library.
 *
 * The Messaging Library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * The Messaging Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the Messaging Library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* Set defualt log level */
int ml_log_level = 2; //2 -> WARNING

void setLogLevel(int ll) {
	ml_log_level = ll;
}
