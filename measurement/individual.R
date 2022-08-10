#!/usr/bin/env Rscript
suppressPackageStartupMessages(library(conflicted))
suppressPackageStartupMessages(library(tidyverse))
suppressPackageStartupMessages(library(patchwork))

options(readr.show_col_types = FALSE)

args = commandArgs(trailingOnly=TRUE)
folder = args[1]
prefix = args[2]
semantics = args[3]
thread_num = strtoi(args[4])
store_size = strtoi(args[5])

dir = getwd()
setwd(paste(dir, folder, sep = "/"))

# Read data.
mixed = list()
for (i in (0:(thread_num - 1))) {
  mixed[[i + 1]] = read_csv(paste(prefix, "_", i, ".csv", sep = ""))
}

# Count violations and prepare for plotting.
backup = list()
violations = 0
version_difference_min = 0
version_difference_max = 0
latency_min = 0
latency_max = 0

out = file("primary_errors.txt", "w")

# Note in R column comes first, then comes row.
for (i in (1:thread_num)) {
  mixed[[i]] %>%
    mutate(semantic_version_diff = remote_version - local_version) ->
    analyzing
  
  primary_violation_flag = FALSE
  request_num = length(analyzing[[1]])
  start = as.integer(request_num * 1/5)
  end = as.integer(request_num * 4/5)
  
  # This script cannot handle eventual consistency.
  if (semantics == "1") { # read-my-writes
    last_write_version <- rep(list(0), store_size)
    for (j in (1:request_num)) {
      if (j > end) {
        break
      }
      index = analyzing[[2]][[j]] + 1
      remote_version = analyzing[[8]][[j]]
      if (analyzing[[3]][[j]] == "0") {
        semantic_version_diff = remote_version - last_write_version[[index]]
        analyzing[[10]][[j]] = semantic_version_diff
        if (semantic_version_diff < 0 && j >= start) {
          violations = violations + 1
        }
      } else {
        version_diff = remote_version - analyzing[[6]][[j]]
        if (version_diff < 0) {
          primary_violation_flag = TRUE
        }
        last_write_version[[index]] = remote_version
      }
    }
  } else if (semantics == "2") { # monotonic-reads
    last_read_version <- rep(list(0), store_size)
    for (j in (1:request_num)) {
      if (j > end) {
        break
      }
      index = analyzing[[2]][[j]] + 1
      remote_version = analyzing[[8]][[j]]
      if (analyzing[[3]][[j]] == "0") {
        semantic_version_diff = remote_version - last_read_version[[index]]
        analyzing[[10]][[j]] = semantic_version_diff
        if (semantic_version_diff < 0 && j >= start) {
          violations = violations + 1
        } else { # Update last read when there's no violation.
          last_read_version[[index]] = remote_version
        }
      } else {
        version_diff = remote_version - analyzing[[6]][[j]]
        if (version_diff < 0) {
          primary_violation_flag = TRUE
        }
      }
    }
  }
  
  if (primary_violation_flag == TRUE) {
    message = paste("primary in thread", i, "failed.", sep = " ")
    print(message)
    write(message, file = "primary_errors.txt", append = TRUE)
  }
  
  # Keep 3/5 of data in the middle.
  analyzing = analyzing[c(start:end),]
  analyzing %>%
    dplyr::filter(operation == "0") ->
    backup[[i]]
  
  version_difference_min = min(version_difference_min, min(backup[[i]][,"semantic_version_diff"]))
  version_difference_max = max(version_difference_max, max(backup[[i]][,"semantic_version_diff"]))
  latency_max = max(latency_max, max(backup[[i]][,"remote_time"]))
}

close(out)

# Plot the graphs (read request only).
p_version = list()
p_latency = list()
for (i in (1:thread_num)) {
  p_version[[i]] <- ggplot(backup[[i]]) +
    labs(title = paste("r", i, sep = ""), y = "semantic version difference") +
    geom_point(aes(x = round, y = semantic_version_diff), size = 0.5) +
    ylim(version_difference_min, version_difference_max)
  
  p_latency[[i]] <- ggplot(backup[[i]]) +
    labs(title = paste("r", i, sep = ""), y = "latency") +
    geom_point(aes(x = round, y = remote_time), size = 0.5) +
    ylim(latency_min, latency_max)
}

# Output violations
fileConn<-file("violations.txt")
writeLines(as.character(violations), fileConn)
close(fileConn)

# Generate graphs.
total = p_version[[1]]
if (thread_num > 1) {
  for (i in (2:thread_num)) {
    total = total + p_version[[i]]
  }
}
for (i in (1:thread_num)) {
  total = total + p_latency[[i]]
}
ggsave(paste(prefix, "_individual.png", sep = ""), plot = total, width = 5000, height = 3000, units = "px")