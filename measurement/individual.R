#!/usr/bin/env Rscript
library(tidyverse)
library(patchwork)

args = commandArgs(trailingOnly=TRUE)
folder = args[1]
prefix = args[2]
threads = args[3]

dir = getwd()
setwd(paste(dir, folder, sep = "/"))

# Read data.
mixed = list()
for (i in (0:(strtoi(threads) - 1))) {
  mixed[[i + 1]] = read_csv(paste(prefix, "_", i, ".csv", sep = ""))
}

# Keep 3/5 of data in the middle.
num_request = length(mixed[[1]][[1]])
for (i in (1:(length(mixed)))) {
  mixed[[i]] = mixed[[i]][c(as.integer(num_request * 1/5):as.integer(num_request * 4/5)),]
}

# Count violations and prepare for plotting.
primary = list()
backup = list()
violations = 0
version_difference_min = 0
version_difference_max = 0
latency_min = 0
latency_max = 0

out = file("primary_violations.txt", "w")

for (i in (1:strtoi(threads))) {
  mixed[[i]] %>%
    filter(operation == "1") %>%
    mutate(version_diff = remote_version - local_version) ->
    primary[[i]]
  if (min(primary[[i]][,"version_diff"]) < 0) {
    message = paste("primary in thread", i, "failed.", sep = " ")
    print(message)
    write(message, file = "primary_violations.txt", append = TRUE)
  }
  
  mixed[[i]] %>%
    filter(operation == "0") %>%
    mutate(version_diff = remote_version - local_version) ->
    backup[[i]]
  summarise(backup[[i]], violations=sum(version_diff<0)) -> temp
  violations = violations + pull(temp)
  version_difference_min = min(version_difference_min, min(backup[[i]][,"version_diff"]))
  version_difference_max = max(version_difference_max, max(backup[[i]][,"version_diff"]))
  latency_max = max(latency_max, max(backup[[i]][,"remote_time"]))
}

close(out)

# Plot the graphs.
p_version = list()
p_latency = list()
for (i in (1:strtoi(threads))) {
  p_version[[i]] <- ggplot(backup[[i]]) +
    labs(title = paste("r", i, sep = ""), y = "version difference") +
    geom_point(aes(x = round, y = version_diff), size = 0.5) +
    ylim(version_difference_min, version_difference_max)
  
  p_latency[[i]] <- ggplot(backup[[i]]) +
    labs(title = paste("r", i, sep = ""), y = "latency") +
    geom_point(aes(x = round, y = remote_time), size = 0.5) +
    ylim(latency_min, latency_max)
}

# Output violations
fileConn<-file("backup_violations.txt")
writeLines(as.character(violations), fileConn)
close(fileConn)

# Generate graphs.
total = p_version[[1]]
if (strtoi(threads) > 1) {
  for (i in (2:strtoi(threads))) {
    total = total + p_version[[i]]
  }
}
for (i in (1:strtoi(threads))) {
  total = total + p_latency[[i]]
}
ggsave(paste(prefix, "_individual.png", sep = ""), plot = total, width = 5000, height = 3000, units = "px")
