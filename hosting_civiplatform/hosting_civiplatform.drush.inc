<?php

/**
 * @file
 * Implement drush hooks for the Platforms module.
 */

/**
 * Implements hook_hosting_TASK_OBJECT_context_options().
 */
function hosting_hosting_civiplatform_context_options(&$task) {
  $task->context_options['server'] = '@server_master';
  $task->context_options['web_server'] = hosting_context_name($task->ref->web_server);
  // Trim the path so we dont pass extra spaces.
  $task->context_options['root'] = trim($task->ref->publish_path, " ");
  $task->context_options['make_working_copy'] = 'null';
  if (empty($task->ref->hosting_name)) {
    $task->ref->hosting_name = hosting_context_name($task->ref->nid);
  }
}

/**
 * Implements hook_drush_context_import().
 */
function hosting_civiplatform_drush_context_import($context, &$node) {
  if ($context->type == 'civiplatform') {
    $node->title = str_replace('civiplatform_', '', trim($context->name, '@'));
    $node->web_server = hosting_drush_import($context->web_server);
    $node->publish_path = $context->root;
  }
}

/**
 * Implements hook_post_verify().
 *
 * Sets the civiplatform verified timestamp, to discren when it was verified.
 * Imports all the profiles and modules into package and package release nodes.
 */
function hosting_civiplatform_post_hosting_verify_task($task, $data) {
  $node = $task->ref;
  if ($node->type == 'civiplatform') {
    // set verified flag on civiplatform, to let it know it has been checked.
    $node->verified = REQUEST_TIME;

    /**
     * If we are verifying a Locked civiplatform (i.e if the publish_path has changed),
     * don't reset the status to Enabled. We don't need to check whether a civiplatform
     * is deleted here for the same reason, because we don't allow a deleted civiplatform
     * to be reverified.
     */
    if ($node->civiplatform_status != HOSTING_CIVIPLATFORM_LOCKED) {
      $node->civiplatform_status = HOSTING_CIVIPLATFORM_ENABLED;
    }
    $node->no_verify = TRUE;
    // Save the civiplatform being verified
    node_save($node);
  }
}

/**
 * Implements hook_hosting_post_DELETE().
 */
function hosting_civiplatform_post_hosting_delete_task($task, $data) {
  // [ML] NOT IMPLEMENTED YET. "as-is", this code would run on a normal drupal platform delete.
  // We probably need a "civicrm-delete" task.
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function hosting_civiplatform_post_hosting_lock_task($task, $data) {
  $task->ref->civiplatform_status = HOSTING_CIVIPLATFORM_LOCKED;
  $task->ref->no_verify = TRUE;
  node_save($task->ref);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function hosting_civiplatform_post_hosting_unlock_task($task, $data) {
  $task->ref->civiplatform_status = HOSTING_CIVIPLATFORM_ENABLED;
  $task->ref->no_verify = TRUE;
  node_save($task->ref);
}

