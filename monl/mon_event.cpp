/***************************************************************************
 *   Copyright (C) 2009 by Robert Birke
 *   robert.birke@polito.it
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 
 ***********************************************************************/

#include "mon_event.h"
#include <event2/event.h>

#include "measure_dispatcher.h"

struct event_base *eventb = NULL;

struct measure_disp_pair {
	class MeasureDispatcher *ptrDispatcher;
	MonHandler mh;
};

void schedule_measure_cb(evutil_socket_t fd, short what, void *arg);
void schedule_publish_cb(evutil_socket_t fd, short what, void *arg);

int schedule_measure(struct timeval *tv, MonMeasure *m) {
	struct measure_disp_pair *m_disp = new struct measure_disp_pair;
	m_disp->ptrDispatcher = m->ptrDispatcher;
	m_disp->mh = m->mh_local;
	return event_base_once(eventb, -1, EV_TIMEOUT, schedule_measure_cb, m_disp, tv);
}

int schedule_publish(struct timeval *tv, MonMeasure *m) {
	struct measure_disp_pair *m_disp = new struct measure_disp_pair;
	m_disp->ptrDispatcher = m->ptrDispatcher;
	m_disp->mh = m->mh_local;
	return event_base_once(eventb, -1, EV_TIMEOUT, schedule_publish_cb, m_disp, tv);
}

void init_mon_event(void *eb) {
	eventb = (struct event_base *) eb;
}

void schedule_measure_cb(evutil_socket_t fd, short what, void *arg) {
	struct measure_disp_pair *m_disp = (struct measure_disp_pair *) arg;
	m_disp->ptrDispatcher->scheduleMeasure(m_disp->mh);
	delete m_disp;
}

void schedule_publish_cb(evutil_socket_t fd, short what, void *arg) {
	struct measure_disp_pair *m_disp = (struct measure_disp_pair *) arg;
	m_disp->ptrDispatcher->schedulePublish(m_disp->mh);
	delete m_disp;
}
