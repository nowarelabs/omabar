SHELL := /bin/bash
DB    := omabar.db
SQLITE3 := sqlite3

.PHONY: help status next blocked in_progress done fail start cancel \
        log comment notes subtasks news subtask notes files \
        stats phases snapshot restore reset rebuild tasks_by_phase \
        blocked_by show dependents

help:
	@echo "=== Omabar Implementation Tracker ==="
	@echo ""
	@echo "Task management:"
	@echo "  make status                 Show overall progress summary"
	@echo "  make tasks_by_phase         List all tasks grouped by phase"
	@echo "  make next                   Show the next unblocked task to work on"
	@echo "  make blocked                Show all blocked tasks (with reasons)"
	@echo "  make in_progress            Show all in-progress tasks"
	@echo "  make show TASK=<id>         Show full details for one task"
	@echo "  make start TASK=<id>        Mark task as in_progress"
	@echo "  make done TASK=<id>         Mark task as completed"
	@echo "  make fail TASK=<id> BLOCKER=\"reason\"  Mark task as blocked"
	@echo "  make retry TASK=<id>        Unblock a blocked task (back to pending)"
	@echo "  make resume TASK=<id>       Resume an in_progress task"
	@echo "  make cancel TASK=<id>       Cancel a task"
	@echo ""
	@echo "Logging & notes:"
	@echo "  make log TASK=<id>          Show all logs for a task"
	@echo "  make note TASK=<id> MSG=\"...\"  Add a note to a task"
	@echo "  make notes TASK=<id>        Show accumulated notes for a task"
	@echo ""
	@echo "Task breakdown:"
	@echo "  make subtasks TASK=<id>        List subtasks for a task"
	@echo "  make news TASK=<id> TITLE=\"...\" DESC=\"...\"  Create subtask"
	@echo "  make subdone TASK=<id> SUB=<n>  Mark subtask as done"
	@echo ""
	@echo "Process management:"
	@echo "  make stats                   Per-phase progress statistics"
	@echo "  make unblocked               List all pending tasks that have unblocked dependencies inline"
	@echo "  make snapshots               List database snapshots"
	@echo "  make snapshot                Create a backup of the database"
	@echo ""
	@echo "Rebuild:"
	@echo "  make reset                   Reset all task statuses to pending"
	@echo "  make rebuild                 Recreate the database from create_db.sh"
	@echo ""
	@echo "Dependency tools:"
	@echo "  make blocked_by TASK=<id>      Show what blocks this task"
	@echo "  make dependents TASK=<id>      Show what depends on this task"

# ------------------------------------------------------------------
# Task management
# ------------------------------------------------------------------

status:
	@echo "=== Omabar Progress ==="
	@sqlite3 -column -header $(DB) \
		"SELECT phase, phase_name, \
			SUM(CASE WHEN status='completed' THEN 1 ELSE 0 END) AS done, \
			SUM(CASE WHEN status='pending' THEN 1 ELSE 0 END) AS pending, \
			SUM(CASE WHEN status='in_progress' THEN 1 ELSE 0 END) AS in_progress, \
			SUM(CASE WHEN status='blocked' THEN 1 ELSE 0 END) AS blocked, \
			COUNT(*) AS total \
		FROM tasks GROUP BY phase ORDER BY phase;"
	@echo ""
	@sqlite3 -column $(DB) \
		"SELECT 'TOTAL', '', \
			SUM(CASE WHEN status='completed' THEN 1 ELSE 0 END), \
			SUM(CASE WHEN status='pending' THEN 1 ELSE 0 END), \
			SUM(CASE WHEN status='in_progress' THEN 1 ELSE 0 END), \
			SUM(CASE WHEN status='blocked' THEN 1 ELSE 0 END), \
			COUNT(*) FROM tasks;"

tasks_by_phase:
	@sqlite3 -header -column $(DB) \
		"SELECT id, '[' || CASE status \
			WHEN 'completed' THEN 'X' \
			WHEN 'in_progress' THEN '>' \
			WHEN 'blocked' THEN '!' \
			ELSE ' ' END || ']' AS st, \
			title, \
			COALESCE(depends_on, '') AS depends \
		 FROM tasks WHERE phase = $(if $(PHASE),$(PHASE),1) ORDER BY id;" 2>/dev/null || true
	@echo ""
	@echo "Available phases: 1..8. Usage: make tasks_by_phase PHASE=2"

next:
	@if [ -z "$(TASK)" ]; then \
		sqlite3 -header -column $(DB) \
			"SELECT t.id, t.phase || ':' || t.phase_name AS phase, t.priority, t.title \
			 FROM tasks t \
			 WHERE t.status = 'pending' \
			   AND NOT EXISTS ( \
			     SELECT 1 \
			     FROM json_each('[' || (CASE WHEN t.depends_on='' THEN '' ELSE t.depends_on END) || ']') j \
			     JOIN tasks dep ON dep.id = CAST(j.value AS INTEGER) \
			     WHERE dep.status != 'completed' \
			   ) \
			 ORDER BY t.phase ASC, t.id ASC LIMIT 10;"; \
	else \
		echo "Dependencies for task $(TASK):"; \
		sqlite3 -column $(DB) \
			"SELECT dep.id, dep.title, dep.status FROM tasks t \
			 JOIN json_each('[' || (CASE WHEN t.depends_on='' THEN '' ELSE t.depends_on END) || ']') AS j \
			 JOIN tasks dep ON dep.id = CAST(j.value AS INTEGER) \
			 WHERE t.id = $(TASK);"; \
	fi

blocked:
	@sqlite3 -header -column $(DB) \
		"SELECT id, phase, title, blocker FROM tasks \
		 WHERE status = 'blocked' ORDER BY id;"

in_progress:
	@sqlite3 -header -column $(DB) \
		"SELECT id, phase, title, updated_at FROM tasks \
		 WHERE status = 'in_progress' ORDER BY id;"

show:
	@if [ -z "$(TASK)" ]; then echo "Usage: make show TASK=<id>"; exit 1; fi
	@sqlite3 -header -column $(DB) \
		"SELECT id, title, phase_name, status, priority, depends_on \
		 FROM tasks WHERE id=$(TASK);"
	@echo ""
	@echo "Description:"
	@sqlite3 $(DB) "SELECT description FROM tasks WHERE id=$(TASK);"
	@echo ""
	@echo "Files created:   "
	@sqlite3 $(DB) "SELECT COALESCE(files_created,'') FROM tasks WHERE id=$(TASK);"
	@echo "Files modified:  "
	@sqlite3 $(DB) "SELECT COALESCE(files_modified,'') FROM tasks WHERE id=$(TASK);"
	@if [ "`sqlite3 $(DB) "SELECT COUNT(*) FROM subtasks WHERE parent_id=$(TASK);"`" -gt 0 ]; then \
		echo ""; \
		echo "Subtasks:"; \
		sqlite3 -header -column $(DB) \
			"SELECT id, status, title FROM subtasks WHERE parent_id=$(TASK) ORDER BY id;"; \
	fi

start:
	@if [ -z "$(TASK)" ]; then echo "Usage: make start TASK=<id>"; exit 1; fi
	@sqlite3 $(DB) "UPDATE tasks SET status='in_progress', updated_at=datetime('now') WHERE id=$(TASK);"
	@sqlite3 $(DB) "INSERT INTO task_logs (task_id, action, message) VALUES ($(TASK), 'started', 'Task started; previous context lost, re-read prompt.md and dependencies');"
	@echo "Task $(TASK) marked in_progress."
	@make show TASK=$(TASK)

done:
	@if [ -z "$(TASK)" ]; then echo "Usage: make done TASK=<id>"; exit 1; fi
	@for dep in `sqlite3 $(DB) "SELECT CAST(j.value AS INTEGER) FROM json_each('[' || (SELECT depends_on FROM tasks WHERE id=$(TASK)) || ']') j;"` ; do \
		stat=`sqlite3 $(DB) "SELECT status FROM tasks WHERE id=$$dep;"`; \
		echo "  dependency $$dep: $$stat"; \
		if [ "$$stat" != "completed" ]; then \
			echo "ERROR: dependency $$dep is not completed."; \
			exit 1; \
		fi; \
	done
	@sqlite3 $(DB) "UPDATE tasks SET status='completed', updated_at=datetime('now') WHERE id=$(TASK);"
	@sqlite3 $(DB) "INSERT INTO task_logs (task_id, action, message) VALUES ($(TASK), 'completed', 'Task completed');"
	@echo "Task $(TASK) marked completed."
	@echo ""
	@echo "Next recommended task(s):"
	@$(MAKE) next TASK=

fail:
	@if [ -z "$(TASK)" -o -z "$(BLOCKER)" ]; then echo "Usage: make fail TASK=<id> BLOCKER=\"reason\""; exit 1; fi
	@sqlite3 $(DB) "UPDATE tasks SET status='blocked', blocker='$(BLOCKER)', updated_at=datetime('now') WHERE id=$(TASK);"
	@sqlite3 $(DB) "INSERT INTO task_logs (task_id, action, message) VALUES ($(TASK), 'blocked', '$(BLOCKER)');"
	@echo "Task $(TASK) blocked: $(BLOCKER)"

retry:
	@if [ -z "$(TASK)" ]; then echo "Usage: make retry TASK=<id>"; exit 1; fi
	@sqlite3 $(DB) "UPDATE tasks SET status='pending', blocker='', updated_at=datetime('now') WHERE id=$(TASK);"
	@sqlite3 $(DB) "INSERT INTO task_logs (task_id, action, message) VALUES ($(TASK), 'note', 'Unblocked: back to pending');"
	@echo "Task $(TASK) unblocked, back to pending."

resume:
	@if [ -z "$(TASK)" ]; then echo "Usage: make resume TASK=<id>"; exit 1; fi
	@sqlite3 $(DB) "UPDATE tasks SET status='in_progress', updated_at=datetime('now') WHERE id=$(TASK);"
	@echo "Task $(TASK) resumed."

cancel:
	@if [ -z "$(TASK)" ]; then echo "Usage: make cancel TASK=<id>"; exit 1; fi
	@sqlite3 $(DB) "UPDATE tasks SET status='cancelled', updated_at=datetime('now') WHERE id=$(TASK);"
	@sqlite3 $(DB) "INSERT INTO task_logs (task_id, action, message) VALUES ($(TASK), 'note', 'Task cancelled');"
	@echo "Task $(TASK) cancelled."

# ------------------------------------------------------------------
# Logging & notes
# ------------------------------------------------------------------

log:
	@if [ -z "$(TASK)" ]; then echo "Usage: make log TASK=<id>"; exit 1; fi
	@sqlite3 -header -column $(DB) \
		"SELECT id, action, created_at, message FROM task_logs WHERE task_id=$(TASK) ORDER BY id;"

note:
	@if [ -z "$(TASK)" -o -z "$(MSG)" ]; then echo "Usage: make note TASK=<id> MSG=\"...\""; exit 1; fi
	@sqlite3 $(DB) "INSERT INTO task_logs (task_id, action, message) VALUES ($(TASK), 'note', '$(MSG)');"
	@sqlite3 $(DB) "UPDATE tasks SET notes = CASE WHEN notes='' THEN '$(MSG)' ELSE notes || char(10) || '$(MSG)' END, updated_at=datetime('now') WHERE id=$(TASK);"
	@echo "Note added to task $(TASK)."

notes:
	@if [ -z "$(TASK)" ]; then echo "Usage: make notes TASK=<id>"; exit 1; fi
	@sqlite3 $(DB) "SELECT COALESCE(notes, '(no notes)') FROM tasks WHERE id=$(TASK);"

# ------------------------------------------------------------------
# Subtasks
# ------------------------------------------------------------------

subtasks:
	@if [ -z "$(TASK)" ]; then echo "Usage: make subtasks TASK=<id>"; exit 1; fi
	@sqlite3 -header -column $(DB) \
		"SELECT id, status, title FROM subtasks WHERE parent_id=$(TASK) ORDER BY id;"

news:
	@if [ -z "$(TASK)" -o -z "$(TITLE)" ]; then echo "Usage: make news TASK=<id> TITLE=\"...\" DESC=\"...\""; exit 1; fi
	@sqlite3 $(DB) "INSERT INTO subtasks (parent_id, title, description) VALUES ($(TASK), '$(TITLE)', '$(DESC)');"
	@sqlite3 $(DB) "INSERT INTO task_logs (task_id, action, message) VALUES ($(TASK), 'subtask_created', '$(TITLE)');"
	@echo "Subtask added to task $(TASK)."
	@make subtasks TASK=$(TASK)

subdone:
	@if [ -z "$(TASK)" -o -z "$(SUB)" ]; then echo "Usage: make subdone TASK=<id> SUB=<subtask-id>"; exit 1; fi
	@sqlite3 $(DB) "UPDATE subtasks SET status='completed' WHERE id=$(SUB) AND parent_id=$(TASK);"
	@echo "Subtask $(SUB) of task $(TASK) completed."
	@done_count=$$(sqlite3 $(DB) "SELECT COUNT(*) FROM subtasks WHERE parent_id=$(TASK) AND status='completed';"); \
	total_count=$$(sqlite3 $(DB) "SELECT COUNT(*) FROM subtasks WHERE parent_id=$(TASK);"); \
	if [ "$$done_count" -eq "$$total_count" ] && [ "$$total_count" -gt 0 ]; then \
		echo "All subtasks complete! Run 'make done TASK=$(TASK)' to mark the task complete."; \
	fi

# ------------------------------------------------------------------
# Process management
# ------------------------------------------------------------------

stats:
	@echo "=== Per-Phase Statistics ==="
	@sqlite3 -header -column $(DB) \
		"SELECT phase, phase_name, \
			SUM(CASE WHEN status='completed' THEN 1 ELSE 0 END) AS done, \
			SUM(CASE WHEN status='in_progress' THEN 1 ELSE 0 END) AS wip, \
			SUM(CASE WHEN status='blocked' THEN 1 ELSE 0 END) AS blocked, \
			SUM(CASE WHEN status='pending' THEN 1 ELSE 0 END) AS pending, \
			COUNT(*) AS total, \
			CAST(ROUND(SUM(CASE WHEN status='completed' THEN 1 ELSE 0 END) * 100.0 / COUNT(*)) AS INT) AS pct \
		FROM tasks GROUP BY phase ORDER BY phase;"
	@echo ""
	@echo "=== Progress by Priority ==="
	@sqlite3 -header -column $(DB) \
		"SELECT priority, \
			SUM(CASE WHEN status='completed' THEN 1 ELSE 0 END) AS done, \
			COUNT(*) AS total \
		FROM tasks GROUP BY priority ORDER BY priority;"

snapshot:
	@cp $(DB) $(DB).snap.$$(date +%Y%m%d_%H%M%S)
	@echo "Snapshot created: $(DB).snap.$$(date +%Y%m%d_%H%M%S)"

snapshots:
	@ls -1 $(DB).snap.* 2>/dev/null || echo "No snapshots found."

reset:
	@echo "WARNING: This will reset ALL task statuses to pending."
	@read -p "Type 'yes' to confirm: " ans; \
	if [ "$$ans" = "yes" ]; then \
		sqlite3 $(DB) "UPDATE tasks SET status='pending', blocker='', updated_at=datetime('now');"; \
		sqlite3 $(DB) "DELETE FROM task_logs;"; \
		sqlite3 $(DB) "UPDATE subtasks SET status='pending';"; \
		echo "All tasks reset to pending."; \
	else echo "Aborted."; fi

rebuild:
	@echo "Recreating database from create_db.sh..."
	@./create_db.sh

unblocked:
	@sqlite3 -header -column $(DB) \
		"SELECT t.id, 'phase ' || t.phase AS phase, t.title, t.depends_on AS deps \
		 FROM tasks t \
		 WHERE t.status='pending' \
		   AND NOT EXISTS ( \
		     SELECT 1 \
		     FROM json_each('[' || (CASE WHEN t.depends_on='' THEN '' ELSE t.depends_on END) || ']') j \
		     JOIN tasks dep ON dep.id = CAST(j.value AS INTEGER) \
		     WHERE dep.status != 'completed' \
		   ) \
		 ORDER BY t.phase, t.id LIMIT 20;"

blocked_by:
	@if [ -z "$(TASK)" ]; then echo "Usage: make blocked_by TASK=<id>"; exit 1; fi
	@echo "This task depends on:"
	@sqlite3 -header -column $(DB) \
		"SELECT dep.id, dep.title, dep.status FROM tasks t \
		 JOIN json_each('[' || (CASE WHEN t.depends_on='' THEN '' ELSE t.depends_on END) || ']') AS j \
		 JOIN tasks dep ON dep.id = CAST(j.value AS INTEGER) \
		 WHERE t.id = $(TASK);"
	@echo ""
	@echo "Uncompleted dependencies:"
	@sqlite3 -header -column $(DB) \
		"SELECT dep.id, dep.title, dep.status FROM tasks t \
		 JOIN json_each('[' || (CASE WHEN t.depends_on='' THEN '' ELSE t.depends_on END) || ']') AS j \
		 JOIN tasks dep ON dep.id = CAST(j.value AS INTEGER) \
		 WHERE t.id = $(TASK) AND dep.status != 'completed';"

dependents:
	@if [ -z "$(TASK)" ]; then echo "Usage: make dependents TASK=<id>"; exit 1; fi
	@echo "Tasks that depend on $(TASK):"
	@sqlite3 -header -column $(DB) \
		"SELECT t.id, t.title, t.status FROM tasks t \
		 WHERE t.depends_on != '' \
		   AND (',' || t.depends_on || ',') LIKE '%,$(TASK),%' \
		 ORDER BY t.id;"