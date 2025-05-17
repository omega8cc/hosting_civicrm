<?php

/**
 * @file
 * Site nodeapi implementations.
 */

/**
 * Implements hook_node_info().
 */
function hosting_civisite_node_info() {
  $types["civisite"] = array(
    "type" => 'civisite',
    "name" => 'CiviCRM Site',
    'base' => 'hosting_civisite',
    "has_title" => TRUE,
    "title_label" => 'Domain name',
    "description" => 'CiviCRM Standalone site',
    "has_body" => 0,
    "body_label" => '',
    "min_word_count" => 0,
  );
  return $types;
}

/**
 * Implements hook_view().
 */
function hosting_civisite_view(&$node, $teaser = FALSE) {
  hosting_set_breadcrumb($node);
  $node->content['info']['#prefix'] = '<div id="hosting-civisite-info" class="hosting-info-list">';
  if ($node->civisite_status == HOSTING_SITE_ENABLED) {
    $node->content['info']['link'] = array(
      '#markup' => _hosting_civisite_goto_link($node),
      '#weight' => -10,
    );
  }

  if (is_numeric($node->client)) {
    $node->content['info']['client'] = array(
      '#type' => 'item',
      '#title' => t('Client'),
      '#markup' => _hosting_node_link($node->client),
      '#weight' => 5,
    );
  }

  $node->content['info']['verified'] = array(
    '#type' => 'item',
    '#title' => t('Verified'),
    '#markup' => hosting_format_interval($node->verified),
  );

  $node->content['info']['civiplatform'] = array(
    '#type' => 'item',
    '#title' => t('Platform'),
    '#markup' => _hosting_node_link($node->civiplatform),
  );

  if ($node->profile) {
    $node->content['info']['profile'] = array(
      '#type' => 'item',
      '#title' => t('Install profile'),
      '#markup' => _hosting_node_link($node->profile),
    );
  }
  if ($node->civisite_language) {
    $node->content['info']['civisite_language'] = array(
      '#type' => 'item',
      '#title' => t('Language'),
      '#markup' => _hosting_language_name($node->civisite_language),
    );
  }

  if ($node->nid) {
    $node->content['info']['status'] = array(
      '#type' => 'item',
      '#title' => t('Status'),
      '#markup' => _hosting_civisite_status($node),
    );
  }

  if ($node->db_server) {
    $node->content['info']['db_server'] = array(
      '#type' => 'item',
      '#title' => t('Database server'),
      '#markup' => _hosting_node_link($node->db_server),
    );
  }

  if ($node->db_name) {
    $node->content['info']['db_name'] = array(
      '#type' => 'item',
      '#title' => t('Database name'),
      '#markup' => check_plain($node->db_name),
    );
  }

  // Support hosting_alias
  if (function_exists('hosting_alias_node_view')) {
    $node->type = 'site';
    hosting_alias_node_view($node, 'full', 'xx');
    $node->type = 'civisite';
  }

  $node->content['info']['#suffix'] = '</div>';

  if ($node->nid) {
    $node->content['tasks_view'] = array(
      '#type' => 'item',
      '#markup' => hosting_task_table($node),
      '#prefix' => '<div id="hosting-task-list">',
      '#suffix' => '</div>',
      '#weight' => 10,
    );
    $settings['hostingTaskRefresh'] = array(
      'nid' => $node->nid,
      'changed' => $node->changed,
    );
    drupal_add_js($settings, array('type' => 'setting', 'scope' => JS_DEFAULT));

    drupal_add_js(drupal_get_path('module', 'hosting_task') . '/hosting_task.js');
  }
  return $node;
}

/**
 * Implements hook_nodeapi_delete_revision().
 */
function hosting_nodeapi_civisite_delete_revision(&$node) {
  db_delete('hosting_civisite')
    ->condition('vid', $node->vid)
    ->execute();
}

/**
 * Implements hook_delete().
 */
function hosting_civisite_delete($node) {
  db_delete('hosting_civisite')
    ->condition('nid', $node->nid)
    ->execute();
  db_delete('hosting_package_instance')
    ->condition('rid', $node->nid)
    ->execute();
  hosting_context_delete($node->nid);
  hosting_task_delete_related_tasks($node->nid);
}

/**
 * Implements hook_nodeapi().
 */
function hosting_civisite_nodeapi_civisite_presave(&$node) {
  // Domain names are case-insensitive.
  $node->title = strtolower(trim($node->title));
}

/**
 * Implements hook_insert().
 */
function hosting_civisite_insert(&$node) {
  if ($node->type != 'civisite') {
    return;
  }

  $client = hosting_get_client($node->client);
  $node->client = $client->nid;
  $node->civisite_language = isset($node->civisite_language) ? $node->civisite_language : 'en';
  // If the cron_key is set use it, otherwise generate a new one.
  $node->cron_key = isset($node->cron_key) ? $node->cron_key : '';
  // Ensure that the last_cron value is set.
  $node->last_cron = isset($node->last_cron) ? $node->last_cron : 0;
  // Provide a dummy profile, e.g. for hosting-import.
  $node->profile = isset($node->profile) ? $node->profile : 0;

  $id = db_insert('hosting_civisite')
    ->fields(array(
      'vid' => $node->vid,
      'nid' => $node->nid,
      'client' => $node->client,
      'db_server' => $node->db_server,
      'db_name' => isset($node->db_name) ? $node->db_name : '',
      'civiplatform' => $node->civiplatform,
      // 'profile' => $node->profile,
      'language' => $node->civisite_language,
      'last_cron' => $node->last_cron,
      'cron_key' => $node->cron_key,
      'status' => isset($node->civisite_status) ? $node->civisite_status : HOSTING_SITE_QUEUED,
      'verified' => isset($node->verified) ? $node->verified : 0,
    ))
    ->execute();

  if (function_exists('hosting_alias_insert')) {
    hosting_alias_insert($node);
  }
}

/**
 * Implements hook_node_insert().
 */
function hosting_civisite_node_insert($node) {
  if ($node->type == "civisite") {
    if ((empty($node->old_vid))) {
      hosting_context_register($node->nid, isset($node->hosting_name) ? $node->hosting_name : $node->title);
      if (isset($node->import)) {
        hosting_add_task($node->nid, 'import');
      }
      else {
        hosting_add_task($node->nid, 'civicrm-install');
      }
    }
  }
}

/**
 * Implements hook_update().
 */
function hosting_civisite_update(&$node) {
  // If this is a new node or we're adding a new revision.
  if (!empty($node->revision)) {
    hosting_civisite_insert($node);
  }
  else {
    $client = hosting_get_client($node->client);
    $node->client = $client->nid;
    if ($node->civisite_status == HOSTING_SITE_DELETED) {
      $node->no_verify = TRUE;
    }

    db_update('hosting_civisite')
      ->fields(array(
        'client' => $node->client,
        'db_server' => $node->db_server,
        'db_name' => $node->db_name,
        'civiplatform' => $node->civiplatform,
        'last_cron' => $node->last_cron,
        'cron_key' => $node->cron_key,
        'status' => $node->civisite_status,
        'profile' => $node->profile,
        'language' => $node->civisite_language,
        'verified' => $node->verified,
      ))
      ->condition('vid', $node->vid)
      ->execute();
  }

  if (function_exists('hosting_alias_update')) {
    hosting_alias_update($node);
  }
}

/**
 * Implements hook_node_update().
 */
function hosting_civisite_node_update($node) {
  if ($node->type == "civisite") {
    if ((isset($node->no_verify) && $node->no_verify) == FALSE) {
      hosting_add_task($node->nid, 'civicrm-verify');
    }
  }
}

/**
 * Implements hook_load().
 */
function hosting_civisite_load($nodes) {
  foreach ($nodes as $nid => &$node) {
    if ($node->type == 'civisite') {
      $additions = db_query('SELECT  client, db_server, db_name, civiplatform, profile, language as civisite_language, last_cron, cron_key, status AS civisite_status, verified FROM {hosting_civisite} WHERE vid = :vid', array(':vid' => $node->vid))->fetch();
      foreach ($additions as $property => &$value) {
        $node->$property = $value;
      }

      if (function_exists('hosting_alias_get_aliases')) {
        // Copied from hosting_alias.module function hosting_alias_node_load().
        $nodes[$nid]->redirection = db_query("SELECT redirection FROM {hosting_site_alias} WHERE vid = :vid", array(':vid' => $node->vid))->fetchField();
        // Only retrieves custom aliases, as they are all that can be modified.
        $nodes[$nid]->aliases = hosting_alias_get_aliases($node, HOSTING_ALIAS_CUSTOM);
        $nodes[$nid]->aliases_automatic = hosting_alias_get_aliases($node, HOSTING_ALIAS_AUTOMATIC);
      }
    }
  }
}

/**
 * Alter the node types on which the Hosting module operates.
 *
 * @param array $types
 *   An array of strings retresentikng node types.
 */
function hosting_civisite_hosting_context_node_types_alter(&$types) {
  $types[] = 'civisite';
}
