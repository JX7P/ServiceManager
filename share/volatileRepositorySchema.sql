/**
 * Markedly simpler than that of the persistent is the volatile repository's
 * schema, for it does not have snapshots, but only live properties whose
 * changes are instantly visible.
 *
 * Two kinds of live instance are distinguished:
 * - the one is a persistent instance; it has a reference to its parent
 * instance ID in the persistent repository, from which it inherits the "Live"
 * snapshot's properties.
 * - the other is a transient instance; it has a reference to a parent service
 * ID in the persistent repository, from which it inherits all the service-level
 * properties. 
 */

BEGIN TRANSACTION;
CREATE TABLE "LiveInstances" (
	"LiveInstanceID"					INTEGER NOT NULL UNIQUE,
	/* The instance's full name (type$service:instance) */
	"Name"								TEXT NOT NULL,
	/* A transient instance must have a parent ServiceID only. */ 
	"PersistentFK_Parent_ServiceID"		INTEGER,
	/* A persistent instance must have a parent InstanceID only. */
	"PersistentFK_Parent_InstanceID"	INTEGER,
	PRIMARY KEY("LiveInstanceID" AUTOINCREMENT)
);
CREATE TABLE "Metadata" (
	"Version"	INTEGER NOT NULL
);
COMMIT;
