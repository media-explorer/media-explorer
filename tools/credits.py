#!/usr/bin/env python
# coding=utf-8

import csv

class Stats:
  def __init__(self):
    self.average = 0
    self.samples = []

  def feed_sample(self, name, kind, duration, credits):

    # If the credits timestamp happens after the duration then something is
    # wrong
    if duration - credits < 0:
      print ("The credits start time for %s is larger than the duration of the "
             "media. Discarding" % name)

    # discard the samples where it's obvious the credits have been cut. This
    # will allow us to get a more meaningful average
    if (duration - credits <= 5):
      return

    self.samples.append (float (credits) / duration)

  def get_average(self):
    return sum(self.samples) / len(self.samples)

stats = Stats()
reader = csv.reader (open('credits.db', 'rb'), delimiter=';')

for line in reader:
  name = line[0]
  kind = line[1]
  duration = int(line[2])
  credits = int(line[3])
  stats.feed_sample(name, kind, duration, credits)

print ("Magic credit start percentage %f" % stats.get_average())
