<template>
  <div class="text-center img-margin">
    <img class="zoomable img-shadow" :src="$withBase(src)" :alt="alt" />
    <div class="no-mp sub-text">
      <slot>{{alt}}</slot>
    </div>
  </div>
</template>

<script>
export default {
  props: ["src", "alt"],
  methods: {}
};
</script>

<style>
</style>